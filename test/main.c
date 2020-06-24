#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "./test_pager.h"

int main(int argc, char const *argv[]) {
    printf("Row size = %d\n", ROW_SIZE);
    printf("MAX_ROW_PER_PAGE = %d\n", MAX_ROWS_PER_PAGE);
    return 0;
}