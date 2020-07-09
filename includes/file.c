#include "./file.h"

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/**
 * Size of file is always times of 4096 bytes. 
 */
void CreateFileIfNotExists(const char *file, off_t size) {
    if (file == NULL || strlen(file) == 0) {
        EXIT_ERROR("file is not exists.\n");
    }
    if (access(file, F_OK) == 0) {
        printf("%s is already exists.\n", file);
        if (access(file, R_OK) == 0) {
            printf("%s can read,", file);
        }
        if (access(file, W_OK) == 0) {
            printf("write");
        }
        printf(".\n");
    } else {
        printf("%s is not exists, it will be create later.\n", file);
        // 创建一个文件，大小为一个pagesize。
        FILE *fp = fopen(file, "wb+");
        if (fp == NULL) {
            EXIT_ERROR("Cannot create file.\n");
        }
        fseek(fp, size - 1, SEEK_SET);
        fputc(1, fp);  // 在PAGE_SIZE-1处放置一个整数
        fclose(fp);
    }
}

int OpenFile(const char *file) {
    assert(file != NULL);
    assert(strlen(file) > 0);

    int fd = open(file, O_RDWR);  // O_RDWR，以读写模式打开；O_WRONLY，以只写模式打开；O_RDONLY，以只读模式打开
    if (fd == -1) {
        EXIT_ERROR("Must provide an existed file.\n");
    }
    return fd;
}

void CloseFile(int fd) {
    if (fd == -1) {
        EXIT_ERROR("Must provide an existed file.\n");
    }
    if (close(fd) == -1) {
        EXIT_ERROR("Failed to close file.\n");
    }
}

off_t FileLength(int fd) {
    return lseek(fd, 0L, SEEK_END);
}