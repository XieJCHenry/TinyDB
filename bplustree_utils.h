#ifndef BPLUSTREE_UTILS_H
#define BPLUSTREE_UTISL_H

#include "./bplustree.h"

void PrintNode(BPlusTreeNode *curNode);
void PrintAllNodes(BPlusTreeNode *root);

static BPlusTreeNode *CreateBuffer();
static void DestroyBuffer(BPlusTreeNode *buffer);

#endif
