#include <stdio.h>
#include "tablero.h"

static int fallos = 0;

static void verificar(const char *nombre, int ok) {
    printf("[%s] %s\n", ok ? "OK" : "FALLA", nombre);
    if (!ok) fallos++;
}

static void limpiar(Tablero *t) {
    inicializar_tablero(t);
    for (int i = 0; i < NODOS; i++) t->ocup[i] = VACIO;
    for (int i = 0; i < TOTAL_PERROS; i++) t->pos_perro[i] = -1;
    t->pos_puma = -1;
    t->perros_capturados = 0;
}

static void puma_en(Tablero *t, int nodo) {
    t->ocup[nodo] = FICHA_PUMA;
    t->pos_puma = nodo;
}

static void perro_en(Tablero *t, int id, int nodo) {
    t->ocup[nodo] = id;
    t->pos_perro[id] = nodo;
}

int main(void) {
    Tablero t;

    inicializar_tablero(&t);
    mostrar_tablero(&t);
    verificar("el trono (2,4) toca la punta", tablero_adyacente(&t, tablero_nodo(2, 4), NODO_TT) == 1);
    verificar("el centro no toca la punta", !tablero_adyacente(&t, tablero_nodo(2, 2), NODO_TT));
    verificar("tablero inicial con 12 perros y puma en el centro",
              t.ocup[tablero_nodo(2, 2)] == FICHA_PUMA && t.perros_capturados == 0 &&
              t.pos_perro[11] >= 0 && !tablero_puma_acorralado(&t));

    inicializar_tablero(&t);
    verificar("puma avanza a casilla libre adyacente",
              tablero_mover(&t, FICHA_PUMA, tablero_nodo(2, 2), tablero_nodo(2, 3)) == 1);
    verificar("movimiento no adyacente se rechaza",
              tablero_mover(&t, FICHA_PUMA, tablero_nodo(2, 3), tablero_nodo(4, 4)) == 0);
    verificar("no se mueve a casilla ocupada",
              tablero_mover(&t, 0, tablero_nodo(0, 0), tablero_nodo(1, 0)) == 0);

    limpiar(&t);
    puma_en(&t, tablero_nodo(2, 2));
    perro_en(&t, 0, tablero_nodo(2, 3));
    verificar("salto ortogonal valido", tablero_salto_valido(&t, tablero_nodo(2, 2), tablero_nodo(2, 3), tablero_nodo(2, 4)) == 1);
    verificar("saltar captura al perro",
              tablero_saltar(&t, tablero_nodo(2, 2), tablero_nodo(2, 3), tablero_nodo(2, 4)) == 1 &&
              t.perros_capturados == 1 && t.ocup[tablero_nodo(2, 3)] == VACIO &&
              t.pos_puma == tablero_nodo(2, 4) && t.pos_perro[0] == -1);

    limpiar(&t);
    puma_en(&t, tablero_nodo(2, 2));
    perro_en(&t, 0, tablero_nodo(2, 3));
    perro_en(&t, 1, tablero_nodo(2, 4));
    verificar("salto bloqueado si el destino esta ocupado",
              tablero_saltar(&t, tablero_nodo(2, 2), tablero_nodo(2, 3), tablero_nodo(2, 4)) == 0 &&
              t.perros_capturados == 0);

    limpiar(&t);
    puma_en(&t, tablero_nodo(0, 4));
    perro_en(&t, 0, NODO_TL);
    verificar("salto en diagonal por el trono (0,4)->TL->TT",
              tablero_saltar(&t, tablero_nodo(0, 4), NODO_TL, NODO_TT) == 1 && t.pos_puma == NODO_TT);

    limpiar(&t);
    puma_en(&t, tablero_nodo(0, 4));
    perro_en(&t, 0, tablero_nodo(1, 4));
    verificar("salto no alineado se rechaza (no es linea recta)",
              tablero_salto_valido(&t, tablero_nodo(0, 4), tablero_nodo(1, 4), NODO_TL) == 0);

    limpiar(&t);
    puma_en(&t, NODO_TT);
    perro_en(&t, 0, NODO_TL);
    perro_en(&t, 1, NODO_TR);
    perro_en(&t, 2, tablero_nodo(2, 4));
    perro_en(&t, 3, tablero_nodo(0, 4));
    perro_en(&t, 4, tablero_nodo(4, 4));
    perro_en(&t, 5, tablero_nodo(2, 3));
    verificar("puma acorralado sin movimientos ni saltos", tablero_puma_acorralado(&t) == 1);

    limpiar(&t);
    puma_en(&t, NODO_TT);
    perro_en(&t, 0, NODO_TL);
    perro_en(&t, 1, NODO_TR);
    perro_en(&t, 2, tablero_nodo(2, 4));
    perro_en(&t, 4, tablero_nodo(4, 4));
    verificar("puma con un salto disponible no esta acorralado", tablero_puma_acorralado(&t) == 0);

    limpiar(&t);
    puma_en(&t, NODO_TT);
    perro_en(&t, 0, NODO_TL);
    perro_en(&t, 1, NODO_TR);
    verificar("puma con una casilla libre no esta acorralado", tablero_puma_acorralado(&t) == 0);

    int px[TOTAL_PERROS] = {0,1,2,3,4,0,1,2,3,4,0,0};
    int py[TOTAL_PERROS] = {0,0,0,0,0,1,1,1,1,1,2,0};
    verificar("colocar rechaza perros duplicados", tablero_colocar(&t, 4, 4, px, py) == -1);
    verificar("colocar rechaza coordenadas fuera de rango", tablero_colocar(&t, 9, 9, NULL, NULL) == -1);

    printf("\nfallos=%d\n", fallos);
    return fallos ? 1 : 0;
}
