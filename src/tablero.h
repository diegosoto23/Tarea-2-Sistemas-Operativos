
#ifndef TABLERO_H
#define TABLERO_H

#define FILAS 5
#define COLUMNAS 5
#define TOTAL_PERROS 12

typedef struct {
    int fila;
    int columna;
} Posicion;

typedef struct {
    Posicion puma;
    Posicion perros[TOTAL_PERROS];
    int perros_activos[TOTAL_PERROS];
    int perros_capturados;
} Tablero;

void inicializar_tablero(Tablero *tablero);
void mostrar_tablero(const Tablero *tablero);

#endif
