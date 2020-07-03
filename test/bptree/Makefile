CC =  gcc
CFLAG = -Wall -g 
OPTIMIZE = -O0

main: file.o bplustree.o artuls.o node2disk.o
	$(CC) $(CFLAGS) $(OPTIMIZE) file.o bplustree.o artuls.o node2disk.o -o main

file.o: ../../includes/file.c
	$(CC) $(CFLAGS) $(OPTIMIZE) -c ../../includes/file.c

bplustree.o: ../../src/bplustree.c
	$(CC) $(CFLAG) $(OPTIMIZE) -c ../../src/bplustree.c

artuls.o: ../../utils/artuls.c
	$(CC) $(CFLAG) $(OPTIMIZE) -c ../../utils/artuls.c

node2disk.o: node2disk.c
	$(CC) $(CFLAG) $(OPTIMIZE) -c node2disk.c

.PHONY:clean
clean:
	rm *.o
