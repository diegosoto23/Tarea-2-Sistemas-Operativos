
#include <stdio.h>
#include "tablero.h"

void inicializar_tablero(Tablero *tablero) {
    tablero->puma.fila = 2;
    tablero->puma.columna = 2;
    tablero->perros_capturados = 0;

    for (int i = 0; i < TOTAL_PERROS; i++) {
        tablero->perros_activos[i] = 1;
        tablero->perros[i].fila = -1;
        tablero->perros[i].columna = -1;
    }
}

void mostrar_tablero(const Tablero *tablero) {
    printf("\nPosicion del Puma: (%d, %d)\n",
           tablero->puma.fila,
           tablero->puma.columna);

    printf("Perros capturados: %d\n",
           tablero->perros_capturados);
}
