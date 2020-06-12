#ifndef ARRAY_UTILS_H
#define ARRAY_UTILS_H

#include <stdint.h>


void Array_Reverse(uint64_t* array, uint64_t len);
void Array_Print(uint64_t* array, uint64_t len);

uint64_t* Array_Fill_Zero(uint64_t length);
uint64_t* Array_Fill_One(uint64_t length);
uint64_t* Array_Fill_Digit(uint64_t digit, uint64_t length);
uint64_t* Array_Fill_Range(uint64_t low, uint64_t high);
uint64_t* Array_Rands_Distinct(uint64_t low, uint64_t high, uint64_t length);
uint64_t* Array_Rands_Duplicate(uint64_t low, uint64_t high, uint64_t length);

#endif