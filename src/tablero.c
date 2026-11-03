#include <stdio.h>
#include <string.h>
#include "tablero.h"

int tablero_nodo(int x, int y) { return y * COLUMNAS + x; }

static int t_x(int nodo) { return nodo % COLUMNAS; }
static int t_y(int nodo) { return nodo / COLUMNAS; }

static void arista(Tablero *t, int a, int b) {
    t->ady[a][t->nady[a]++] = b;
    t->ady[b][t->nady[b]++] = a;
}

static void construir(Tablero *t) {
    memset(t, 0, sizeof *t);
    for (int y = 0; y < FILAS; y++)
        for (int x = 0; x < COLUMNAS; x++) {
            t->px[tablero_nodo(x, y)] = x;
            t->py[tablero_nodo(x, y)] = y;
        }
    t->px[NODO_TL] = 1; t->py[NODO_TL] = 5;
    t->px[NODO_TR] = 3; t->py[NODO_TR] = 5;
    t->px[NODO_TT] = 2; t->py[NODO_TT] = 6;

    for (int y = 0; y < FILAS; y++)
        for (int x = 0; x < COLUMNAS; x++) {
            if (x < COLUMNAS - 1) arista(t, tablero_nodo(x, y), tablero_nodo(x + 1, y));
            if (y < FILAS - 1) arista(t, tablero_nodo(x, y), tablero_nodo(x, y + 1));
        }
    arista(t, NODO_TL, tablero_nodo(0, 4));
    arista(t, NODO_TL, tablero_nodo(1, 4));
    arista(t, NODO_TR, tablero_nodo(3, 4));
    arista(t, NODO_TR, tablero_nodo(4, 4));
    arista(t, NODO_TL, NODO_TR);
    arista(t, NODO_TT, NODO_TL);
    arista(t, NODO_TT, NODO_TR);
    arista(t, NODO_TT, tablero_nodo(2, 4));

    for (int i = 0; i < NODOS; i++) t->ocup[i] = VACIO;
    for (int i = 0; i < TOTAL_PERROS; i++) t->pos_perro[i] = -1;
    t->pos_puma = -1;
}

static int en_rango(int x, int y) { return x >= 0 && x < COLUMNAS && y >= 0 && y < FILAS; }

static const int ORDEN[25][2] = {
    {0,0},{1,0},{2,0},{3,0},{4,0},
    {0,1},{1,1},{2,1},{3,1},{4,1},
    {0,2},{4,2},{1,2},{3,2},{2,2},
    {0,3},{1,3},{2,3},{3,3},{4,3},
    {0,4},{1,4},{2,4},{3,4},{4,4}
};

int tablero_colocar(Tablero *t, int puma_x, int puma_y, const int *perros_x, const int *perros_y) {
    int ocup[NODOS];
    int dx[TOTAL_PERROS], dy[TOTAL_PERROS];
    for (int i = 0; i < NODOS; i++) ocup[i] = VACIO;
    if (!en_rango(puma_x, puma_y)) return -1;
    ocup[tablero_nodo(puma_x, puma_y)] = FICHA_PUMA;

    if (perros_x && perros_y) {
        for (int d = 0; d < TOTAL_PERROS; d++) { dx[d] = perros_x[d]; dy[d] = perros_y[d]; }
    } else {
        int d = 0;
        for (int i = 0; i < 25 && d < TOTAL_PERROS; i++) {
            if (ORDEN[i][0] == puma_x && ORDEN[i][1] == puma_y) continue;
            dx[d] = ORDEN[i][0];
            dy[d] = ORDEN[i][1];
            d++;
        }
    }
    for (int d = 0; d < TOTAL_PERROS; d++) {
        if (!en_rango(dx[d], dy[d])) return -1;
        int n = tablero_nodo(dx[d], dy[d]);
        if (ocup[n] != VACIO) return -1;
        ocup[n] = d;
    }
    memcpy(t->ocup, ocup, sizeof ocup);
    t->pos_puma = tablero_nodo(puma_x, puma_y);
    for (int d = 0; d < TOTAL_PERROS; d++) t->pos_perro[d] = tablero_nodo(dx[d], dy[d]);
    t->perros_capturados = 0;
    return 0;
}

void inicializar_tablero(Tablero *t) {
    construir(t);
    tablero_colocar(t, 2, 2, NULL, NULL);
}

int tablero_adyacente(const Tablero *t, int a, int b) {
    if (a < 0 || a >= NODOS) return 0;
    for (int i = 0; i < t->nady[a]; i++)
        if (t->ady[a][i] == b) return 1;
    return 0;
}

static void poner(Tablero *t, int ficha, int nodo) {
    t->ocup[nodo] = ficha;
    if (ficha == FICHA_PUMA) t->pos_puma = nodo;
    else t->pos_perro[ficha] = nodo;
}

int tablero_mover(Tablero *t, int ficha, int desde, int hasta) {
    if (desde < 0 || desde >= NODOS || hasta < 0 || hasta >= NODOS) return 0;
    if (t->ocup[desde] != ficha) return 0;
    if (t->ocup[hasta] != VACIO) return 0;
    if (!tablero_adyacente(t, desde, hasta)) return 0;
    t->ocup[desde] = VACIO;
    poner(t, ficha, hasta);
    return 1;
}

int tablero_salto_valido(const Tablero *t, int desde, int sobre, int hasta) {
    if (desde < 0 || desde >= NODOS || sobre < 0 || sobre >= NODOS) return 0;
    if (hasta < 0 || hasta >= NODOS) return 0;
    if (t->ocup[desde] != FICHA_PUMA) return 0;
    if (t->ocup[sobre] < 0 || t->ocup[sobre] >= TOTAL_PERROS) return 0;
    if (t->ocup[hasta] != VACIO) return 0;
    if (!tablero_adyacente(t, desde, sobre) || !tablero_adyacente(t, sobre, hasta)) return 0;
    return t->px[hasta] == 2 * t->px[sobre] - t->px[desde] &&
           t->py[hasta] == 2 * t->py[sobre] - t->py[desde];
}

int tablero_saltar(Tablero *t, int desde, int sobre, int hasta) {
    if (!tablero_salto_valido(t, desde, sobre, hasta)) return 0;
    int perro = t->ocup[sobre];
    t->ocup[desde] = VACIO;
    t->ocup[sobre] = VACIO;
    t->pos_perro[perro] = -1;
    poner(t, FICHA_PUMA, hasta);
    t->perros_capturados++;
    return 1;
}

int tablero_puma_acorralado(const Tablero *t) {
    int p = t->pos_puma;
    if (p < 0) return 0;
    for (int i = 0; i < t->nady[p]; i++) {
        int v = t->ady[p][i];
        if (t->ocup[v] == VACIO) return 0;
        for (int j = 0; j < t->nady[v]; j++)
            if (tablero_salto_valido(t, p, v, t->ady[v][j])) return 0;
    }
    return 1;
}

void tablero_nombre_nodo(int nodo, char *out, size_t n) {
    if (nodo == NODO_TL) snprintf(out, n, "TRONO_LEFT");
    else if (nodo == NODO_TR) snprintf(out, n, "TRONO_RIGHT");
    else if (nodo == NODO_TT) snprintf(out, n, "TRONO_TOP");
    else snprintf(out, n, "%d,%d", t_x(nodo), t_y(nodo));
}

static char simbolo(const Tablero *t, int n) {
    if (t->ocup[n] == VACIO) return '.';
    return t->ocup[n] == FICHA_PUMA ? 'P' : 'D';
}

void tablero_a_texto(const Tablero *t, char *out, size_t n) {
    int u = snprintf(out, n, "      %c\n    %c   %c\n", simbolo(t, NODO_TT), simbolo(t, NODO_TL), simbolo(t, NODO_TR));
    for (int y = FILAS - 1; y >= 0 && u >= 0 && (size_t)u < n; y--) {
        u += snprintf(out + u, n - (size_t)u, "  ");
        for (int x = 0; x < COLUMNAS && (size_t)u < n; x++)
            u += snprintf(out + u, n - (size_t)u, "%c ", simbolo(t, tablero_nodo(x, y)));
        if ((size_t)u < n) u += snprintf(out + u, n - (size_t)u, "\n");
    }
}

void mostrar_tablero(const Tablero *t) {
    char buf[256];
    tablero_a_texto(t, buf, sizeof buf);
    printf("%s", buf);
}
