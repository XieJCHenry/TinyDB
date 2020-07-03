#ifndef TINYDB_FILE_H
#define TINYDB_FILE_H
#include <sys/types.h>

#include "./global.h"

TINYDB_API void CreateFileIfNotExists(const char *file, off_t size);
TINYDB_API int OpenFile(const char *file);
TINYDB_API void CloseFile(int fd);
#endif