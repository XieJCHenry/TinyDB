#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../../includes/file.h"
#include "../../src/bplustree.h"
#include "../../utils/artuls.h"

// 1. 将节点从内存中序列化到文件中
void serializeLeafNode2Disk() {
    // BpTreeLeafNode *leaf = (BpTreeLeafNode *)calloc(1, sizeof(BpTreeLeafNode));
    BpTreeLeafNodePersist *leaf = calloc(1, sizeof(BpTreeLeafNodePersist));
    leaf->keyNum                = 10;
    leaf->flags                 = 15;  // is root,is leaf, has modified
    leaf->parentOffset          = -1;
    leaf->prevOffset            = 12;
    leaf->nextOffset            = 14;

    // leaf->keys           = (KEY *)calloc(leaf->keyNum, sizeof(KEY));
    // leaf->values         = (VAL *)calloc(leaf->keyNum, sizeof(VAL));
    uint64_t low, high;
    low = 13, high = 23;
    uint64_t *arr = Array_Rands_Distinct(low, high, high - low);
    Array_Print(arr, high - low);
    leaf->keys   = arr;
    leaf->values = arr;

    const char *file = "db";
    off_t fileSize   = 4096;
    CreateFileIfNotExists(file, fileSize);
    int fd = OpenFile(file);
    lseek(fd, 0L, SEEK_SET);
    // write header
    uint32_t headerSize = sizeof(uint8_t) + sizeof(uint64_t) + sizeof(off_t) * 3;
    write(fd, leaf, headerSize);
    // write keys
    void *buffer = calloc(leaf->keyNum * 2, sizeof(KEY));
    memcpy(buffer, leaf->keys, sizeof(KEY) * leaf->keyNum);
    memcpy(buffer + leaf->keyNum, leaf->values, sizeof(VAL) * leaf->keyNum);
    write(fd, buffer, sizeof(KEY) * leaf->keyNum * 2);

    CloseFile(fd);
}

void deserializeLeafNode2Memory() {
    void *buffer = malloc(sizeof(char) * 4096);
    int fd       = OpenFile("db");
    lseek(fd, 0L, SEEK_SET);
    read(fd, buffer, 4096);
    BpTreeLeafNodePersist *leaf;
    leaf = (BpTreeLeafNodePersist *)buffer;

    printf("leaf->flags = %d\n", leaf->flags);
    printf("leaf->keyNum = %ld\n", leaf->keyNum);
    CloseFile(fd);
}

int main(int argc, char const *argv[]) {
    // serializeLeafNode2Disk();
    deserializeLeafNode2Memory();
    return 0;
}