#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "servidor.h"

static int entero_positivo(const char *s, int *out) {
    char *fin;
    long v = strtol(s, &fin, 10);
    if (*s == '\0' || *fin != '\0' || v < 1 || v > 100000) return 0;
    *out = (int)v;
    return 1;
}

int main(int argc, char **argv) {
    int n, m;
    if (argc < 3 || argc > 4 || !entero_positivo(argv[1], &n) || !entero_positivo(argv[2], &m)) {
        fprintf(stderr, "uso: %s N M [config.txt]\n", argv[0]);
        return 2;
    }
    Config cfg;
    config_defecto(&cfg);
    if (argc == 4 && config_cargar(&cfg, argv[3]) != 0) return 2;
    return servidor_ejecutar(n, m, &cfg);
}
