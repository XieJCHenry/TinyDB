CC = g++
CFLAGS = -Wall -g
OPTIMIZE = -O0

test_pager: test_pager.o pager.o
	$(CC) $(CFLAGS) test_pager.o pager.o -o test_pager

test_pager.o: test_pager.c
	$(CC) $(CFLAGS) -c test_pager.c

pager.o: pager.c
	$(CC) $(CFLAGS) -c pager.c

.PHONY:clean
clean:
	rm *.o
