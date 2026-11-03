#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include "registro.h"

static pthread_mutex_t candado = PTHREAD_MUTEX_INITIALIZER;

void registro(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    pthread_mutex_lock(&candado);
    vprintf(fmt, ap);
    fflush(stdout);
    pthread_mutex_unlock(&candado);
    va_end(ap);
}
