#ifndef TABLERO_H
#define TABLERO_H

#include <stddef.h>

#define FILAS 5
#define COLUMNAS 5
#define TOTAL_PERROS 12
#define NODOS 28
#define NODO_TL 25
#define NODO_TR 26
#define NODO_TT 27
#define VACIO (-1)
#define FICHA_PUMA 100
#define MAX_ADY 6

typedef struct {
    int ady[NODOS][MAX_ADY];
    int nady[NODOS];
    int px[NODOS];
    int py[NODOS];
    int ocup[NODOS];
    int pos_perro[TOTAL_PERROS];
    int pos_puma;
    int perros_capturados;
} Tablero;

int tablero_nodo(int x, int y);
void inicializar_tablero(Tablero *t);
int tablero_colocar(Tablero *t, int puma_x, int puma_y, const int *perros_x, const int *perros_y);
int tablero_adyacente(const Tablero *t, int a, int b);
int tablero_mover(Tablero *t, int ficha, int desde, int hasta);
int tablero_salto_valido(const Tablero *t, int desde, int sobre, int hasta);
int tablero_saltar(Tablero *t, int desde, int sobre, int hasta);
int tablero_puma_acorralado(const Tablero *t);
void tablero_nombre_nodo(int nodo, char *out, size_t n);
void tablero_a_texto(const Tablero *t, char *out, size_t n);
void mostrar_tablero(const Tablero *t);

#endif
