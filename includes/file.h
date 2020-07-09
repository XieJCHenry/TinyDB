#ifndef TINYDB_FILE_H
#define TINYDB_FILE_H
#include <sys/types.h>

#include "./global.h"

#define S_SEEK(fd, off, seek) (        \
    do {                                  \
        if (lseek(fd, off, seek) == -1) { \
            EXIT_ERROR("Error Seek.\n");  \
        }                                 \
    } while (0))

#define S_READ(fd, buffer, size) (         \
    do {                                      \
        if (read(fd, buffer, size) != size) { \
            EXIT_ERROR("Error Read.\n");      \
        }                                     \
    } while (0))

#define S_WRITE(fd, buffer, size) (         \
    do {                                       \
        if (write(fd, buffer, size) != size) { \
            EXIT_ERROR("Error write.\n");      \
        }                                      \
    } while (0))

// #define 

TINYDB_API void CreateFileIfNotExists(const char *file, off_t size);
TINYDB_API int OpenFile(const char *file);
TINYDB_API void CloseFile(int fd);
TINYDB_API off_t FileLength(int fd);
#endif