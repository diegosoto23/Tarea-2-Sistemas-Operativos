#ifndef CONFIG_H
#define CONFIG_H

#include "tablero.h"

typedef struct {
    unsigned seed;
    int delay_min;
    int delay_max;
    int puma_x;
    int puma_y;
    int max_movs;
    int nperros;
    int dx[TOTAL_PERROS];
    int dy[TOTAL_PERROS];
    int disc_puma;
    int disc_perro;
} Config;

void config_defecto(Config *c);
int config_cargar(Config *c, const char *ruta);

#endif
