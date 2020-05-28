/* Simple Implementation of B plus Tree -- all in memory */

#ifndef BPLUSTREE_H
#define BPLUSTREE_H

#include <stdint.h>
#include <stdbool.h>

#define LEVEL                4
#define MAX_RECORDS_PER_NODE (LEVEL - 1)

typedef enum {
    LeafNode,
    InternalNode,
    RootNode
} NodeType;

typedef struct t_record {
    uint64_t key;
    uint64_t value;
} Record;

typedef struct bplustree_node {
    bool isRoot, isLeaf;
    // NodeType type;
    uint64_t keyNum;  // record the num of records or childs which depends on the type
    uint64_t *keys;
    uint64_t *values;
    struct bplustree_node **childs;
    struct bplustree_node *parent;
    struct bplustree_node *prev;
    struct bplustree_node *next;
} BPlusTreeNode;

// typedef struct bplustree {
//     BPlusTreeNode *root;
//     BPlusTreeNode *smallestNode;
// } BPlusTree;

extern void BPlusTree_Init();
extern void BPlusTree_Destroy();
extern void BPlusTree_Insert(uint64_t key, uint64_t value);
extern uint64_t BPlusTree_Select(uint64_t key);
extern uint64_t *BPlusTree_Select_Range(uint64_t key, uint64_t range,uint64_t *length);
extern uint64_t BPlusTree_Modify(uint64_t key, uint64_t newValue);
extern uint64_t BPlusTree_Delete(uint64_t key);

extern void BPlusTree_PrintTree();

#endif
