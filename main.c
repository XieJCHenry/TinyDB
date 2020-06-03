#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "./bplustree.h"

uint64_t num = 1000 * 10000;

void TestInsert();

int main(int argc, char* argv[]) {
    clock_t start, end;
    BPlusTree_Init();
    start = clock();
    TestInsert();
    end = clock();
    printf("Insert %ld records takes %f seconds.\n", num, (double)(end - start) / CLOCKS_PER_SEC);
    // pause();
    start = clock();
    BPlusTree_Destroy();
    end = clock();
    printf("Free %ld nodes takes %f seconds.\n", BPlusTree_AllNodes(), (double)(end - start) / CLOCKS_PER_SEC);

    return 0;
}

/**
 * B+Tree has been initialized success.
Insert 100000 records takes 0.010000 seconds.
B+Tree has been destroyed.
Free 100000 nodes takes 0.010000 seconds.
 * 
 * B+Tree has been initialized success.
Insert 1000000 records takes 0.220000 seconds.
B+Tree has been destroyed.
Free 1000000 nodes takes 0.040000 seconds.
 * 
 * 
 * B+Tree has been initialized success.
Insert 1000000 records takes 0.210000 seconds.
B+Tree has been destroyed.
Free 1000000 nodes takes 0.050000 seconds.
 * 
 * 
 * B+Tree has been initialized success.
Insert 10000000 records takes 2.590000 seconds.
B+Tree has been destroyed.
Free 10000000 nodes takes 0.440000 seconds.
 * 
 */
void TestInsert() {
    uint64_t i;
    for (i = 1; i <= num; i++) {
        BPlusTree_Insert(i, i);
        // BPlusTree_AllRecords();
    }
}
