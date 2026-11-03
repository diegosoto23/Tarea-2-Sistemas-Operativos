#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "juego.h"
#include "registro.h"

#define HILOS (TOTAL_PERROS + 1)
#define CAPTURAS_VICTORIA 6
#define TABLERO_CADA 10
#define PERRO_QUE_FALLA 3
#define PILA_HILO (512 * 1024)

struct Mesa {
    int id;
    Config cfg;
    unsigned semilla;
    Tablero t;
    pthread_mutex_t mtx;
    pthread_cond_t cond;
    int terminada;
    int cancelar;
    int caido_puma;
    int caido_perros;
    Ganador ganador;
    const char *motivo;
    int movimientos;
    struct timespec inicio;
};

typedef struct {
    Mesa *m;
    int ficha;
} ArgHilo;

static long ms_desde(const Mesa *m) {
    struct timespec ahora;
    clock_gettime(CLOCK_MONOTONIC, &ahora);
    return (long)(ahora.tv_sec - m->inicio.tv_sec) * 1000L + (ahora.tv_nsec - m->inicio.tv_nsec) / 1000000L;
}

static int esperar(Mesa *m, int ms) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    ts.tv_sec += ms / 1000;
    ts.tv_nsec += (long)(ms % 1000) * 1000000L;
    if (ts.tv_nsec >= 1000000000L) {
        ts.tv_sec++;
        ts.tv_nsec -= 1000000000L;
    }
    while (!m->terminada) {
        if (pthread_cond_timedwait(&m->cond, &m->mtx, &ts) == ETIMEDOUT) break;
    }
    return m->terminada;
}

static int retardo(const Config *c, unsigned *sem) {
    return c->delay_min + (int)(rand_r(sem) % (unsigned)(c->delay_max - c->delay_min + 1));
}

static void terminar(Mesa *m, Ganador g, const char *motivo) {
    if (m->terminada) return;
    m->terminada = 1;
    m->ganador = g;
    m->motivo = motivo;
    pthread_cond_broadcast(&m->cond);
}

static void evaluar(Mesa *m) {
    if (m->terminada) return;
    if (m->t.perros_capturados >= CAPTURAS_VICTORIA) terminar(m, GANA_PUMA, "el Puma capturo 6 perros");
    else if (tablero_puma_acorralado(&m->t)) terminar(m, GANA_PERROS, "el Puma quedo acorralado");
    else if (m->movimientos >= m->cfg.max_movs) terminar(m, GANA_PERROS, "limite de movimientos alcanzado");
}

static void anunciar(Mesa *m, const char *quien, int desde, int hasta, int perro) {
    char a[16], b[16], buf[256];
    tablero_nombre_nodo(desde, a, sizeof a);
    tablero_nombre_nodo(hasta, b, sizeof b);
    if (perro >= 0)
        registro("[mesa %d] mov %d: %s %s -> %s CAPTURA perro %d (capturas=%d)\n", m->id, m->movimientos, quien, a, b, perro, m->t.perros_capturados);
    else
        registro("[mesa %d] mov %d: %s %s -> %s\n", m->id, m->movimientos, quien, a, b);
    if (perro >= 0 || m->movimientos % TABLERO_CADA == 0) {
        tablero_a_texto(&m->t, buf, sizeof buf);
        registro("[mesa %d] tablero tras mov %d:\n%s", m->id, m->movimientos, buf);
    }
}

static void mover_puma(Mesa *m, unsigned *sem) {
    Tablero *t = &m->t;
    int p = t->pos_puma;
    int sobre[MAX_ADY * MAX_ADY], dest[MAX_ADY * MAX_ADY], ns = 0;
    for (int i = 0; i < t->nady[p]; i++) {
        int v = t->ady[p][i];
        for (int j = 0; j < t->nady[v]; j++) {
            int w = t->ady[v][j];
            if (tablero_salto_valido(t, p, v, w)) {
                sobre[ns] = v;
                dest[ns] = w;
                ns++;
            }
        }
    }
    if (ns > 0) {
        int k = (int)(rand_r(sem) % (unsigned)ns);
        int perro = t->ocup[sobre[k]];
        if (!tablero_saltar(t, p, sobre[k], dest[k])) return;
        m->movimientos++;
        anunciar(m, "PUMA", p, dest[k], perro);
        evaluar(m);
        return;
    }
    int libres[MAX_ADY], nl = 0;
    for (int i = 0; i < t->nady[p]; i++)
        if (t->ocup[t->ady[p][i]] == VACIO) libres[nl++] = t->ady[p][i];
    if (nl == 0) return;
    int hasta = libres[rand_r(sem) % (unsigned)nl];
    if (!tablero_mover(t, FICHA_PUMA, p, hasta)) return;
    m->movimientos++;
    anunciar(m, "PUMA", p, hasta, -1);
    evaluar(m);
}

static int dist2(const Tablero *t, int a, int b) {
    int dx = t->px[a] - t->px[b];
    int dy = t->py[a] - t->py[b];
    return dx * dx + dy * dy;
}

static void mover_perro(Mesa *m, int id, unsigned *sem) {
    Tablero *t = &m->t;
    int pos = t->pos_perro[id];
    if (pos < 0) return;
    int libres[MAX_ADY], nl = 0;
    for (int i = 0; i < t->nady[pos]; i++)
        if (t->ocup[t->ady[pos][i]] == VACIO) libres[nl++] = t->ady[pos][i];
    if (nl == 0) return;
    int hasta = libres[rand_r(sem) % (unsigned)nl];
    if (rand_r(sem) % 100 < 75) {
        for (int i = 0; i < nl; i++)
            if (dist2(t, libres[i], t->pos_puma) < dist2(t, hasta, t->pos_puma)) hasta = libres[i];
    }
    if (!tablero_mover(t, id, pos, hasta)) return;
    m->movimientos++;
    char nombre[16];
    snprintf(nombre, sizeof nombre, "PERRO %d", id);
    anunciar(m, nombre, pos, hasta, -1);
    evaluar(m);
}

static void *hilo_ficha(void *p) {
    ArgHilo *a = p;
    Mesa *m = a->m;
    int ficha = a->ficha;
    int es_puma = ficha == FICHA_PUMA;
    unsigned sem = m->semilla * 2654435761u + (unsigned)ficha + 1u;
    int limite = es_puma ? m->cfg.disc_puma : (ficha == PERRO_QUE_FALLA ? m->cfg.disc_perro : -1);
    pthread_mutex_lock(&m->mtx);
    while (!m->terminada) {
        if (!es_puma && m->t.pos_perro[ficha] < 0) break;
        if (esperar(m, retardo(&m->cfg, &sem))) break;
        if (limite >= 0 && ms_desde(m) >= limite) {
            if (es_puma) m->caido_puma = 1;
            else m->caido_perros = 1;
            pthread_cond_broadcast(&m->cond);
            break;
        }
        if (es_puma) mover_puma(m, &sem);
        else mover_perro(m, ficha, &sem);
    }
    pthread_mutex_unlock(&m->mtx);
    return NULL;
}

Mesa *mesa_crear(int id, const Config *cfg, unsigned semilla) {
    Mesa *m = calloc(1, sizeof *m);
    if (!m) return NULL;
    m->id = id;
    m->cfg = *cfg;
    m->semilla = semilla;
    inicializar_tablero(&m->t);
    if (tablero_colocar(&m->t, cfg->puma_x, cfg->puma_y, cfg->nperros ? cfg->dx : NULL, cfg->nperros ? cfg->dy : NULL) != 0) {
        free(m);
        return NULL;
    }
    pthread_condattr_t ca;
    pthread_condattr_init(&ca);
    pthread_condattr_setclock(&ca, CLOCK_MONOTONIC);
    pthread_mutex_init(&m->mtx, NULL);
    pthread_cond_init(&m->cond, &ca);
    pthread_condattr_destroy(&ca);
    return m;
}

void mesa_jugar(Mesa *m, Resultado *out) {
    pthread_t h[HILOS];
    ArgHilo a[HILOS];
    int n = 0;
    char buf[256];
    clock_gettime(CLOCK_MONOTONIC, &m->inicio);
    pthread_attr_t at;
    pthread_attr_init(&at);
    pthread_attr_setstacksize(&at, PILA_HILO);

    pthread_mutex_lock(&m->mtx);
    tablero_a_texto(&m->t, buf, sizeof buf);
    registro("[mesa %d] inicia la partida, tablero inicial:\n%s", m->id, buf);
    evaluar(m);
    pthread_mutex_unlock(&m->mtx);

    for (int i = 0; i < HILOS; i++) {
        a[i].m = m;
        a[i].ficha = i < TOTAL_PERROS ? i : FICHA_PUMA;
        if (pthread_create(&h[n], &at, hilo_ficha, &a[i]) != 0) {
            pthread_mutex_lock(&m->mtx);
            terminar(m, GANA_NADIE, "error al crear los hilos de la mesa");
            pthread_mutex_unlock(&m->mtx);
            break;
        }
        n++;
    }

    pthread_mutex_lock(&m->mtx);
    while (!m->terminada && !m->cancelar && !m->caido_puma && !m->caido_perros)
        pthread_cond_wait(&m->cond, &m->mtx);
    if (!m->terminada) {
        if (m->caido_puma) terminar(m, GANA_PERROS, "abandono del Puma");
        else if (m->caido_perros) terminar(m, GANA_PUMA, "abandono de los Perros");
        else terminar(m, GANA_NADIE, "partida cancelada por SIGINT");
    }
    tablero_a_texto(&m->t, buf, sizeof buf);
    registro("[mesa %d] fin: %s (%s) tras %d movimientos, capturas=%d\n%s", m->id,
             m->ganador == GANA_PUMA ? "gana el Puma" : m->ganador == GANA_PERROS ? "ganan los Perros" : "sin ganador",
             m->motivo, m->movimientos, m->t.perros_capturados, buf);
    pthread_mutex_unlock(&m->mtx);

    for (int i = 0; i < n; i++) pthread_join(h[i], NULL);
    pthread_attr_destroy(&at);

    pthread_mutex_lock(&m->mtx);
    out->mesa = m->id;
    out->ganador = m->ganador;
    out->motivo = m->motivo;
    out->movimientos = m->movimientos;
    out->capturas = m->t.perros_capturados;
    pthread_mutex_unlock(&m->mtx);
}

void mesa_cancelar(Mesa *m) {
    pthread_mutex_lock(&m->mtx);
    m->cancelar = 1;
    pthread_cond_broadcast(&m->cond);
    pthread_mutex_unlock(&m->mtx);
}

void mesa_destruir(Mesa *m) {
    if (!m) return;
    pthread_cond_destroy(&m->cond);
    pthread_mutex_destroy(&m->mtx);
    free(m);
}
