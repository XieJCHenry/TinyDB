#include "bplustree_utils.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "bplustree.h"

void PrintNode(BPlusTreeNode *curNode) {
    if (curNode == NULL) {
        return;
    }
    if (curNode->isRoot) {
        printf("Root:");
    }
    if (!curNode->isLeaf && !curNode->isRoot) {
        printf("\nInternal:");
    }
    if (curNode->isLeaf) {
        printf("Leaf:");
    }

    printf("[");
    uint64_t i;
    for (i = 0; i < curNode->keyNum; i++) {
        if (curNode->isLeaf)
            printf("%ld=%ld", curNode->keys[i], curNode->values[i]);
        else
            printf("%ld", curNode->keys[i]);
        if (i != curNode->keyNum - 1) {
            printf(",");
        }
    }
    printf("]");
    printf("; ");
}

void PrintAllNodes(BPlusTreeNode *root) {
    if (root == NULL) {
        return;
    }
    PrintNode(root);
    if (root->isRoot)
        printf("\n");
    if (!root->isLeaf) {
        uint64_t i;
        for (i = 0; i <= root->keyNum; i++) {
            PrintAllNodes(root->childs[i]);
        }
    }
}
