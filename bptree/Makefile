CC = gcc
CFLAGS = -Wall -g -std=c99
OPTIMIZE = -O0

main: main.o bplustree.o artuls.o
	$(CC) $(CFLAGS) $(OPTIMIZE) main.o bplustree.o artuls.o -o main

bplustree.o: bplustree.c
	$(CC) $(CFLAGS) $(OPTIMIZE) -c bplustree.c

artuls.o: utils/artuls.c
	$(CC) $(CFLAGS) $(OPTIMIZE) -c utils/artuls.c

main.o: main.c
	$(CC) $(CFLAGS) $(OPTIMIZE) -c main.c

.PHONY:clean
clean:
	rm *.o
