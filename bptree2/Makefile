CC = g++
CFLAGS = -Wall -g
OPTIMIZE = -O0

main: main.o  file.o bptree.o
	$(CC) $(CFLAGS) $(OPTIMIZE) main.o file.o bptree.o -o main

file.o: ../includes/file.h
	$(CC) $(CFLAGS) $(OPTIMIZE) -c ../includes/file.c

main.o: main.c
	$(CC) $(CFLAGS) $(OPTIMIZE) -c main.c

bptree.o: bptree.c
	$(CC) $(CFLAGS) $(OPTIMIZE) -c bptree.c

.PHONY:clean
clean:
	rm *.o
