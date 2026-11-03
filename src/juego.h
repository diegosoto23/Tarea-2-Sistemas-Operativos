#ifndef JUEGO_H
#define JUEGO_H

#include "config.h"

typedef struct Mesa Mesa;

typedef enum { GANA_NADIE = 0, GANA_PUMA = 1, GANA_PERROS = 2 } Ganador;

typedef struct {
    int mesa;
    Ganador ganador;
    const char *motivo;
    int movimientos;
    int capturas;
} Resultado;

Mesa *mesa_crear(int id, const Config *cfg, unsigned semilla);
void mesa_jugar(Mesa *m, Resultado *out);
void mesa_cancelar(Mesa *m);
void mesa_destruir(Mesa *m);

#endif
