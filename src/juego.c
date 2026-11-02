
#include <stdio.h>
#include "juego.h"

void iniciar_juego(void) {
    Tablero tablero;

    inicializar_tablero(&tablero);

    printf("\nInicializando partida...\n");
    mostrar_tablero(&tablero);
}
