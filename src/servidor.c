#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "juego.h"
#include "registro.h"
#include "servidor.h"

#define ROL_PUMA 0
#define ROL_PERROS 1
#define LLEGADA_MAX_MS 40

typedef struct {
    int id;
    int rol;
    unsigned semilla;
} Cliente;

typedef struct {
    Mesa *mesa;
    int slot;
} Tarea;

static struct {
    sem_t mesas;
    pthread_mutex_t mtx;
    pthread_cond_t cond;
    int n;
    int m;
    int detener;
    int sigint;
    int *cola[2];
    int cabeza[2];
    int cola_fin[2];
    Mesa **activas;
    int partidas;
    int ganadas[3];
} S;

static int en_cola(int rol) { return S.cola_fin[rol] - S.cabeza[rol]; }

static void esperar_ms(int ms) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    ts.tv_sec += ms / 1000;
    ts.tv_nsec += (long)(ms % 1000) * 1000000L;
    if (ts.tv_nsec >= 1000000000L) {
        ts.tv_sec++;
        ts.tv_nsec -= 1000000000L;
    }
    while (!S.detener) {
        if (pthread_cond_timedwait(&S.cond, &S.mtx, &ts) == ETIMEDOUT) break;
    }
}

static void *hilo_cliente(void *arg) {
    Cliente *c = arg;
    int espera = (int)(rand_r(&c->semilla) % (LLEGADA_MAX_MS + 1));
    pthread_mutex_lock(&S.mtx);
    esperar_ms(espera);
    if (!S.detener) {
        S.cola[c->rol][S.cola_fin[c->rol]++] = c->id;
        registro("[servidor] llega el cliente %d como %s\n", c->id, c->rol == ROL_PUMA ? "Puma" : "Perros");
        pthread_cond_broadcast(&S.cond);
    }
    pthread_mutex_unlock(&S.mtx);
    return NULL;
}

static void *hilo_senales(void *arg) {
    (void)arg;
    sigset_t set;
    int sig;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGUSR1);
    for (;;) {
        if (sigwait(&set, &sig) != 0) continue;
        if (sig == SIGUSR1) break;
        pthread_mutex_lock(&S.mtx);
        if (!S.detener) {
            S.detener = 1;
            S.sigint = 1;
            registro("[servidor] SIGINT recibido: se detiene la llegada de jugadores y se concluyen las partidas activas\n");
            for (int i = 0; i < S.m; i++)
                if (S.activas[i]) mesa_cancelar(S.activas[i]);
            pthread_cond_broadcast(&S.cond);
            sem_post(&S.mesas);
        }
        pthread_mutex_unlock(&S.mtx);
    }
    return NULL;
}

static void *hilo_mesa(void *arg) {
    Tarea *t = arg;
    Resultado r;
    mesa_jugar(t->mesa, &r);
    pthread_mutex_lock(&S.mtx);
    S.partidas++;
    S.ganadas[r.ganador]++;
    S.activas[t->slot] = NULL;
    pthread_mutex_unlock(&S.mtx);
    mesa_destruir(t->mesa);
    sem_post(&S.mesas);
    free(t);
    return NULL;
}

int servidor_ejecutar(int n, int m, const Config *cfg) {
    sigset_t bloqueo;
    sigemptyset(&bloqueo);
    sigaddset(&bloqueo, SIGINT);
    sigaddset(&bloqueo, SIGUSR1);
    pthread_sigmask(SIG_BLOCK, &bloqueo, NULL);

    memset(&S, 0, sizeof S);
    S.n = n;
    S.m = m;
    int pares_max = n / 2 + 1;
    Cliente *clientes = calloc((size_t)n, sizeof *clientes);
    pthread_t *hclientes = calloc((size_t)n, sizeof *hclientes);
    pthread_t *hmesas = calloc((size_t)pares_max, sizeof *hmesas);
    S.cola[0] = calloc((size_t)n, sizeof(int));
    S.cola[1] = calloc((size_t)n, sizeof(int));
    S.activas = calloc((size_t)m, sizeof *S.activas);
    if (!clientes || !hclientes || !hmesas || !S.cola[0] || !S.cola[1] || !S.activas) {
        fprintf(stderr, "sin memoria\n");
        free(clientes); free(hclientes); free(hmesas); free(S.cola[0]); free(S.cola[1]); free(S.activas);
        return 1;
    }

    pthread_condattr_t ca;
    pthread_condattr_init(&ca);
    pthread_condattr_setclock(&ca, CLOCK_MONOTONIC);
    pthread_mutex_init(&S.mtx, NULL);
    pthread_cond_init(&S.cond, &ca);
    pthread_condattr_destroy(&ca);
    sem_init(&S.mesas, 0, (unsigned)m);

    pthread_t hsenales;
    int codigo = 0;
    if (pthread_create(&hsenales, NULL, hilo_senales, NULL) != 0) {
        fprintf(stderr, "no se pudo crear el hilo de senales\n");
        codigo = 1;
        goto limpiar;
    }

    registro("[servidor] N=%d clientes, M=%d mesas, seed=%u, retardo=%d..%d ms\n", n, m, cfg->seed, cfg->delay_min, cfg->delay_max);

    int ncreados = 0;
    for (int i = 0; i < n; i++) {
        clientes[i].id = i + 1;
        clientes[i].rol = i % 2 == 0 ? ROL_PUMA : ROL_PERROS;
        clientes[i].semilla = cfg->seed * 2246822519u + (unsigned)i + 7u;
        if (pthread_create(&hclientes[i], NULL, hilo_cliente, &clientes[i]) != 0) break;
        ncreados++;
    }

    int pares = ncreados / 2;
    int formados = 0;
    int nmesas = 0;
    while (formados < pares) {
        pthread_mutex_lock(&S.mtx);
        while (!S.detener && (en_cola(ROL_PUMA) == 0 || en_cola(ROL_PERROS) == 0))
            pthread_cond_wait(&S.cond, &S.mtx);
        int parar = S.detener;
        pthread_mutex_unlock(&S.mtx);
        if (parar) break;

        while (sem_wait(&S.mesas) == -1 && errno == EINTR) {}

        pthread_mutex_lock(&S.mtx);
        if (S.detener) {
            pthread_mutex_unlock(&S.mtx);
            break;
        }
        int cp = S.cola[ROL_PUMA][S.cabeza[ROL_PUMA]++];
        int cd = S.cola[ROL_PERROS][S.cabeza[ROL_PERROS]++];
        formados++;
        int slot = 0;
        while (slot < m && S.activas[slot]) slot++;
        Mesa *mesa = slot < m ? mesa_crear(formados, cfg, cfg->seed + (unsigned)formados) : NULL;
        Tarea *tarea = mesa ? malloc(sizeof *tarea) : NULL;
        if (!tarea) {
            mesa_destruir(mesa);
            sem_post(&S.mesas);
            pthread_mutex_unlock(&S.mtx);
            fprintf(stderr, "no se pudo preparar la mesa %d\n", formados);
            continue;
        }
        tarea->mesa = mesa;
        tarea->slot = slot;
        S.activas[slot] = mesa;
        registro("[matchmaker] mesa %d (slot %d): Puma=cliente %d vs Perros=cliente %d\n", formados, slot, cp, cd);
        if (pthread_create(&hmesas[nmesas], NULL, hilo_mesa, tarea) != 0) {
            S.activas[slot] = NULL;
            mesa_destruir(mesa);
            free(tarea);
            sem_post(&S.mesas);
            pthread_mutex_unlock(&S.mtx);
            fprintf(stderr, "no se pudo crear el hilo de la mesa %d\n", formados);
            continue;
        }
        nmesas++;
        pthread_mutex_unlock(&S.mtx);
    }

    for (int i = 0; i < nmesas; i++) pthread_join(hmesas[i], NULL);
    for (int i = 0; i < ncreados; i++) pthread_join(hclientes[i], NULL);

    pthread_kill(hsenales, SIGUSR1);
    pthread_join(hsenales, NULL);

    registro("[servidor] resumen: partidas=%d, gana Puma=%d, ganan Perros=%d, canceladas=%d, clientes sin partida=%d\n",
             S.partidas, S.ganadas[GANA_PUMA], S.ganadas[GANA_PERROS], S.ganadas[GANA_NADIE], n - 2 * S.partidas);
    if (S.sigint) codigo = 130;

limpiar:
    sem_destroy(&S.mesas);
    pthread_cond_destroy(&S.cond);
    pthread_mutex_destroy(&S.mtx);
    free(clientes);
    free(hclientes);
    free(hmesas);
    free(S.cola[0]);
    free(S.cola[1]);
    free(S.activas);
    return codigo;
}
