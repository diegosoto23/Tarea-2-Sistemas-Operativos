# Torneo de Komikan Concurrente — Tarea 2 de Sistemas Operativos

Integrantes: Cristian Hernández, Diego Soto

## Compilación y uso

```
make
./torneo N M [config.txt]
```

- `N`: número total de clientes. Se emparejan de a dos (un Puma y un Perros). Si N es impar, un cliente queda sin pareja.
- `M`: máximo de mesas activas al mismo tiempo.
- `config.txt` (opcional): parámetros de la partida.

Compilación exigida por el enunciado: `gcc -Wall -Wextra -std=c17 -pthread` (es lo que ejecuta `make`, sin warnings).

`make test` compila y ejecuta las pruebas unitarias del tablero (`tests/test_tablero.c`).

## Archivo de configuración

Pares `CLAVE valor` separados por espacios o saltos de línea. Las líneas que parten con `#` son comentarios y la palabra `CONFIG` se ignora (así acepta el mismo formato de `tests/test_cases.txt`).

| Clave | Significado | Por defecto |
|---|---|---|
| `SEED n` | Semilla aleatoria | 1 |
| `DELAY_MIN ms` / `DELAY_MAX ms` | Retardo entre decisiones de cada ficha | 50 / 300 |
| `PUMA_INIT x y` | Casilla inicial del Puma | 2 2 |
| `PERRO x y` | Casilla inicial de un perro (hay que dar exactamente 12 líneas, o ninguna para usar la disposición por defecto) | por defecto |
| `MAX_MOVS n` | Límite de movimientos por partida | 300 |
| `DISCONNECT_PUMA ms` | Simula la caída del jugador Puma a los ms indicados | desactivado |
| `DISCONNECT_PERRO ms` | Simula la caída del jugador Perros (cae el perro 3) a los ms indicados | desactivado |

Ejemplos en `cfg/`.

## Estructura del código

| Archivo | Contenido |
|---|---|
| `src/tablero.c/.h` | Grafo de 28 nodos (cuadrícula 5x5 + trono), adyacencias, movimiento, salto/captura y detección de Puma acorralado. No usa hilos. |
| `src/config.c/.h` | Lectura y validación del archivo de configuración. |
| `src/registro.c/.h` | Log sincronizado con mutex para que las líneas de distintos hilos no se mezclen. |
| `src/juego.c/.h` | Mesa: Juez, 12 hilos de perros, 1 hilo de Puma, reglas y condiciones de victoria. |
| `src/servidor.c/.h` | Llegada de clientes, Matchmaker, límite de M mesas, manejo de SIGINT y limpieza. |
| `src/main.c` | Validación de argumentos. |

## Diseño de sincronización (resumen; detalle en el informe)

- Cada mesa tiene un `pthread_mutex_t` que protege el tablero y su estado. Validar y aplicar un movimiento ocurre completo dentro de la sección crítica, por lo que si dos fichas quieren la misma casilla solo una la obtiene y la otra falla y reintenta tras otro retardo aleatorio.
- Los retardos y el despertar del Juez usan `pthread_cond_t` con reloj monotónico (`pthread_cond_timedwait`): no hay espera activa y una cancelación despierta a todos de inmediato.
- El límite de M mesas se implementa con un `sem_t` inicializado en M. El Matchmaker espera pasivamente en `sem_wait` cuando las M mesas están ocupadas.
- SIGINT está bloqueada en todos los hilos y la atiende un hilo dedicado con `sigwait`, de modo que el manejo no ocurre dentro de un handler asíncrono.
- Orden global de locks: servidor → mesa → log. Nunca se toma en orden inverso, lo que evita deadlock.

## Supuestos de diseño

El enunciado deja varios puntos abiertos; estas son las decisiones tomadas:

1. El tablero es un grafo de 28 nodos: 25 de la cuadrícula más 3 del trono (`TRONO_LEFT`, `TRONO_RIGHT`, `TRONO_TOP`). Las adyacencias son ortogonales, más las del trono.
2. Un salto solo es válido en línea recta: el Puma salta sobre un perro adyacente hacia la casilla opuesta, que debe ser adyacente al perro y estar libre. No hay capturas múltiples en un solo movimiento.
3. El Puma prioriza capturar cuando puede; si no, se mueve a una casilla libre al azar.
4. Los perros se mueven al azar, con 75 % de probabilidad de acercarse al Puma.
5. Los perros ganan si el Puma queda sin movimientos ni saltos. También ganan si se alcanza `MAX_MOVS` sin que el Puma llegue a 6 capturas (garantiza que toda partida termina).
6. El "jugador Puma" es el hilo de la ficha Puma y el "jugador Perros" es el conjunto de los 12 hilos de perros. Una desconexión se simula con `DISCONNECT_*`: la ficha cae sin retener ningún lock y el Juez declara ganador al oponente por abandono.
7. Los clientes llegan con un retardo aleatorio corto y esperan en colas FIFO (una por rol); el Matchmaker empareja el primero de cada cola.
8. Si el servidor recibe SIGINT, las partidas activas terminan como "sin ganador" y los pares que no alcanzaron mesa no se inician. El proceso termina con código 130.
9. El log imprime cada movimiento y el tablero cada 10 movimientos y en cada captura.

## Pruebas

Las líneas de `tests/test_cases.txt` son un guion de eventos que el programa no interpreta. Cada caso se cubre así:

| Caso | Cómo se verifica |
|---|---|
| 1, 2, 12 | `./torneo 4 2 cfg/rapido.txt`; estrés con `cfg/estres.txt` (retardo 1 ms) |
| 3, 4 (colisión) | Garantizado por el mutex de mesa; ejecutado bajo ThreadSanitizer sin warnings |
| 5 (saturación) | `./torneo 20 2 cfg/saturacion.txt` (se observó un máximo de 2 mesas simultáneas) |
| 6, 7 (captura, movimiento inválido) | `make test` |
| 8 (trono) | `./torneo 2 1 cfg/trono.txt` y `make test` |
| 9, 10 (desconexión) | `cfg/desc_puma.txt`, `cfg/desc_perro.txt` |
| 11 (SIGINT) | `./torneo 10 3 cfg/sigint.txt` y Ctrl+C; el proceso retorna 130 |

Verificaciones adicionales hechas: `-fsanitize=thread` sin advertencias y `valgrind --leak-check=full` sin fugas ni errores, incluso al interrumpir con SIGINT.
