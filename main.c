#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "./bplustree.h"
#include "utils/arutls.h"

/*=========================*/

#define UNIT_TEST
#define RANDOM_INPUT

#ifdef UNIT_TEST
// #define TEST_INSERT
#define TEST_DELETE
#endif

/*=========================*/

#ifdef UNIT_TEST
double FunctionTimeCost(uint64_t times, void (*f)(uint64_t t2));
double VoidFunctionTimeCost(void (*f)(void));
#endif
void TestInsert();
void TestDelete();

int main(int argc, char* argv[]) {
    TestDelete();
    // TestInsert();
    return 0;
}

#ifdef TEST_DELETE
void TestDelete() {
    printf("============Starting Unit Test: TestDelete============\n");
    uint64_t low, high, length;
    low    = 1;
    high   = 101;
    length = high - low;
    uint64_t *keys, *values;
#ifdef RANDOM_INPUT
    keys   = Array_Rands_Distinct(low, high, length);
    values = Array_Rands_Distinct(low, high, length);
#else
    keys   = Array_Fill_Range(low, high);
    values = Array_Fill_Range(low, high);
    Array_Reverse(keys, length);
    Array_Reverse(values, length);
#endif

    Array_Print(keys, length);
    Array_Print(values, length);

    BPlusTree_Init();

    uint64_t i;
    for (i = 0; i < length; i++) {
        BPlusTree_Insert(keys[i], values[i]);
    }

    BPlusTree_PrintTree();

    printf("======================== Delete ========================\n");

    // for (i = 0; i < length; i++) {
    //     BPlusTree_Delete(i + low);
    //     // if (i % 10 == 0) {
    //         BPlusTree_PrintTree();
    //     // }
    // }
    for (i = 0; i < length; i++) {
        // BPlusTree_Delete(keys[i]);
        BPlusTree_Delete(i + low);
        BPlusTree_PrintTree();
    }

    BPlusTree_Destroy();
    free(keys);
    free(values);
    printf("============Exit Unit Test: TestDelete============\n");
}
#endif

#ifdef TEST_INSERT
void TestInsert() {
    printf("============Starting Unit Test: TestInsert============\n");
    uint64_t low, high, length;
    low              = 1;
    high             = 101;
    length           = high - low;
    uint64_t* keys   = Array_Rands_Distinct(low, high, length);
    uint64_t* values = Array_Rands_Distinct(low, high, length);

    Array_Print(keys, length);
    Array_Print(values, length);

    BPlusTree_Init();

    uint64_t i;
    for (i = 0; i < length; i++) {
        BPlusTree_Insert(keys[i], values[i]);
        printf("insert: %ld\n", keys[i]);
        BPlusTree_PrintTree();
    }
}
#endif

double FunctionTimeCost(uint64_t times, void (*f)(uint64_t t2)) {
    clock_t start, end;
    start = clock();
    f(times);
    end = clock();
    return (double)(end - start) / CLOCKS_PER_SEC;
}

double VoidFunctionTimeCost(void (*f)(void)) {
    clock_t start, end;
    start = clock();
    f();
    end = clock();
    return (double)(end - start) / CLOCKS_PER_SEC;
}
