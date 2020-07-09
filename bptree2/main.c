#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "./bptree.h"

void test_New_BpTree();

int main(int argc, char const *argv[]) {
    test_New_BpTree();
    return 0;
}

void test_New_BpTree() {
    BpTree *tree = New_BpTree(NULL);
    printf("tree->datFd = %d\n", tree->datFd);
    printf("tree->freeBlock->max_num = %ld\n", tree->freeBlock->max_num);
    printf("tree->config->dataFileSize = %ld\n", tree->config->dataFileSize);
    printf("tree->config->indexFileSize = %ld\n", tree->config->indexFileSize);
}