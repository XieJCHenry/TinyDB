#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "./arutls.h"

#define CHECK_IF_POSITIVE(digit)                  \
    do {                                          \
        if ((digit) <= 0) {                       \
            printf(#digit " must be postive.\n"); \
            exit(EXIT_FAILURE);                   \
        }                                         \
    } while (0)

void Array_Reverse(uint64_t* array, uint64_t len) {
    uint64_t i;
    for (i = 0; i <= len / 2; i++) {
        uint64_t temp      = array[i];
        array[i]           = array[len - i - 1];
        array[len - i - 1] = temp;
    }
}

void Array_Print(uint64_t* array, uint64_t len) {
    uint64_t i;
    for (i = 0; i < len; i++) {
        printf("%ld", array[i]);
        if (i < len - 1) {
            printf(", ");
        }
    }
    printf("\n");
}

uint64_t* Array_Fill_Zero(uint64_t length) {
    CHECK_IF_POSITIVE(length);
    uint64_t* array = calloc(length, sizeof(uint64_t));
    assert(array != NULL);
    return array;
}

uint64_t* Array_Fill_One(uint64_t length) {
    CHECK_IF_POSITIVE(length);
    uint64_t* array = calloc(length, sizeof(uint64_t));
    assert(array != NULL);

    uint64_t i;
    for (i = 0; i < length; i++) {
        array[i] = 1;
    }
    return array;
}

uint64_t* Array_Fill_Digit(uint64_t digit, uint64_t length) {
    CHECK_IF_POSITIVE(length);
    uint64_t* array = calloc(length, sizeof(uint64_t));
    assert(array != NULL);

    uint64_t i;
    for (i = 0; i < length; i++) {
        array[i] = digit;
    }
    return array;
}

uint64_t* Array_Fill_Range(uint64_t low, uint64_t high) {
    CHECK_IF_POSITIVE(low);
    CHECK_IF_POSITIVE(high);
    CHECK_IF_POSITIVE(high - low);
    uint64_t* array = calloc((high - low), sizeof(uint64_t));
    assert(array != NULL);

    uint64_t i;
    for (i = 0; i < (high - low); i++) {
        array[i] = low + i;
    }

    return array;
}

/**
 * Generate an array filled with distincted elements.
 * 
 * @param low: the lower limit (inlcude)
 * @param high: the higher limit (exclude)
 * @param length:
 * @return array: allocated at heap
 */
uint64_t* Array_Rands_Distinct(uint64_t low, uint64_t high, uint64_t length) {
    uint64_t range, i, j, p;
    range = high - low;
    CHECK_IF_POSITIVE(range);
    uint64_t* array = calloc(range, sizeof(uint64_t));
    assert(array != NULL);

    for (p = 0; p < range; p++) {
        array[p] = p + low;
    }

    srand((int)time(NULL));
    for (p = 0; p < range; p++) {
        i = (rand() % (range));
        j = (rand() % (range));
        if (i != j) {
            uint64_t temp = array[i];
            array[i]      = array[j];
            array[j]      = temp;
        }
    }
    return array;
}

/**
 * Generate an array filled with (probably) duplicated elements.
 * 
 * @param low: the lower limit (inlcude)
 * @param high: the higher limit (exclude)
 * @param length:
 * @return array: allocated at heap
 */
uint64_t* Array_Rands_Duplicate(uint64_t low, uint64_t high, uint64_t length) {
    CHECK_IF_POSITIVE(low);
    CHECK_IF_POSITIVE(high);
    CHECK_IF_POSITIVE(high - low);

    uint64_t p, range;
    range           = high - low;
    uint64_t* array = calloc(range, sizeof(uint64_t));
    assert(array != NULL);

    srand((int)time(NULL));
    for (p = 0; p < range; p++) {
        array[p] = rand() % range + low;
    }
    return array;
}