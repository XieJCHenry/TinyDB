#ifndef TINIdb_GLOBAL_H
#define TINIdb_GLOBAL_H

#define PRIVATE static
#define TINYDB_API extern

#define DEBUG 1

#ifdef DEBUG
#define DEBUG_TEST 1
#else
#define DEBUG_TEST 0
#endif

// #define SIZE_OF_ATTRIBUTE(struct, attr) (sizeof(((struct *)0)->attr))

#define EXIT_ERROR(msg)     \
    do {                    \
        perror(msg);        \
        exit(EXIT_FAILURE); \
    } while (0)

#endif