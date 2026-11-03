#include <stdio.h>
#include <string.h>
#include "config.h"

void config_defecto(Config *c) {
    memset(c, 0, sizeof *c);
    c->seed = 1;
    c->delay_min = 50;
    c->delay_max = 300;
    c->puma_x = 2;
    c->puma_y = 2;
    c->max_movs = 300;
    c->disc_puma = -1;
    c->disc_perro = -1;
}

static int leer_int(FILE *f, int *v) { return fscanf(f, "%d", v) == 1; }

int config_cargar(Config *c, const char *ruta) {
    FILE *f = fopen(ruta, "r");
    if (!f) {
        perror(ruta);
        return -1;
    }
    char tok[64];
    int ok = 1;
    while (ok && fscanf(f, "%63s", tok) == 1) {
        if (tok[0] == '#') {
            int ch;
            while ((ch = fgetc(f)) != '\n' && ch != EOF) {}
        } else if (!strcmp(tok, "CONFIG")) {
            continue;
        } else if (!strcmp(tok, "SEED")) {
            ok = fscanf(f, "%u", &c->seed) == 1;
        } else if (!strcmp(tok, "DELAY_MIN")) {
            ok = leer_int(f, &c->delay_min);
        } else if (!strcmp(tok, "DELAY_MAX")) {
            ok = leer_int(f, &c->delay_max);
        } else if (!strcmp(tok, "MAX_MOVS")) {
            ok = leer_int(f, &c->max_movs);
        } else if (!strcmp(tok, "DISCONNECT_PUMA")) {
            ok = leer_int(f, &c->disc_puma);
        } else if (!strcmp(tok, "DISCONNECT_PERRO")) {
            ok = leer_int(f, &c->disc_perro);
        } else if (!strcmp(tok, "PUMA_INIT")) {
            ok = leer_int(f, &c->puma_x) && leer_int(f, &c->puma_y);
        } else if (!strcmp(tok, "PERRO")) {
            ok = c->nperros < TOTAL_PERROS && leer_int(f, &c->dx[c->nperros]) && leer_int(f, &c->dy[c->nperros]);
            if (ok) c->nperros++;
        } else {
            fprintf(stderr, "%s: parametro desconocido '%s'\n", ruta, tok);
            fclose(f);
            return -1;
        }
    }
    fclose(f);
    if (!ok) {
        fprintf(stderr, "%s: valor invalido en la configuracion\n", ruta);
        return -1;
    }
    if (c->delay_min < 0 || c->delay_max < c->delay_min || c->max_movs < 1) {
        fprintf(stderr, "%s: retardos o MAX_MOVS fuera de rango\n", ruta);
        return -1;
    }
    if (c->nperros != 0 && c->nperros != TOTAL_PERROS) {
        fprintf(stderr, "%s: se requieren exactamente %d lineas PERRO\n", ruta, TOTAL_PERROS);
        return -1;
    }
    Tablero t;
    inicializar_tablero(&t);
    if (tablero_colocar(&t, c->puma_x, c->puma_y, c->nperros ? c->dx : NULL, c->nperros ? c->dy : NULL) != 0) {
        fprintf(stderr, "%s: disposicion inicial invalida\n", ruta);
        return -1;
    }
    return 0;
}
