CC = gcc
CFLAGS = -Wall -g -std=c99

main: main.o bplustree.o bplustree_utils.o
	$(CC) $(CFLAGS) main.o bplustree.o bplustree_utils.o -o main

bplustree.o: bplustree.c
	$(CC) $(CFLAGS) -c bplustree.c

bplustree_utils.o: bplustree_utils.c
	$(CC) $(CFLAGS) -c bplustree_utils.c

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

.PHONY:clean
clean:
	rm *.o
