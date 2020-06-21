#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "./pager.h"

int main(int argc, char const *argv[]) {
    const char *file = "db";
    FILE *fp         = OpenFile(file);
    if (fp != NULL) {
        printf("Success\n");
    } else {
        printf("Failed.\n");
    }
    CloseFile(fp);
    return 0;
}