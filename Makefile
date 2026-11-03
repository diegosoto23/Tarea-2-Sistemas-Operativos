CC = gcc
CFLAGS = -Wall -Wextra -std=c17
LDLIBS = -lpthread
FUENTES = $(wildcard src/*.c)

torneo: $(FUENTES) $(wildcard src/*.h)
	$(CC) $(CFLAGS) -o torneo $(FUENTES) $(LDLIBS)

test_tablero: tests/test_tablero.c src/tablero.c src/tablero.h
	$(CC) $(CFLAGS) -Isrc -o test_tablero tests/test_tablero.c src/tablero.c

test: test_tablero
	./test_tablero

clean:
	rm -f torneo test_tablero
