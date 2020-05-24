CC = gcc
CFLAGS = -Wall -g -std=c99
CUNIT_FLAG = -lcunit

TestBPlusTree: Test_bplustree.o bplustree.o
	$(CC) $(CFLAGS) $(CUNIT_FLAG) Test_bplustree.o bplustree.o -o TestBPlusTree

Test_bplustree.o: Test_bplustree.c 
	$(CC) $(CFLAGS) -c Test_bplustree.c

bplustree.o: ../bplustree.c
	$(CC) $(CFLAGS) -c ../bplustree.c

clean:
	rm *.o