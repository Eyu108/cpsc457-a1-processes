CC=gcc
CFLAGS=-O2 -Wall

all: a1p1 a1p2

a1p1: src/a1p1.c
	$(CC) $(CFLAGS) src/a1p1.c -o a1p1

a1p2: src/a1p2.c
	$(CC) $(CFLAGS) src/a1p2.c -o a1p2 -lm

run1: a1p1
	./a1p1 < inputs/inputfilep1.txt

run2: a1p2
	./a1p2 $(LOWER) $(UPPER) $(N)

clean:
	rm -f a1p1 a1p2 *.o
