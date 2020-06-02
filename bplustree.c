#include "bplustree.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "bplustree_utils.h"

#define MID(x) ((x % 2 == 0) ? (x >> 1) : ((x >> 1) + 1))

BPlusTreeNode *Root;

uint64_t TotalNodes;

static BPlusTreeNode *New_BPlusTreeNode(NodeType type) {
    BPlusTreeNode *node = (BPlusTreeNode *)malloc(sizeof(BPlusTreeNode));
    if (node == NULL) {
        return NULL;
    }
    node->keyNum = 0;
    node->parent = NULL;
    node->next   = NULL;
    node->prev   = NULL;
    if (type == LeafNode) {
        node->keys   = (uint64_t *)malloc(sizeof(uint64_t) * MAX_RECORDS_PER_NODE);
        node->values = (uint64_t *)malloc(sizeof(uint64_t) * MAX_RECORDS_PER_NODE);
        if (node->keys == NULL || node->values == NULL) {
            if (node->keys != NULL) {
                free(node->keys);
            }
            if (node->values != NULL) {
                free(node->values);
            }
            free(node);
            return NULL;
        }
        node->childs = NULL;
        node->isLeaf = true;
        node->isRoot = false;
    } else if (type == InternalNode || type == RootNode) {
        node->keys   = (uint64_t *)malloc(sizeof(uint64_t) * MAX_RECORDS_PER_NODE);
        node->childs = (BPlusTreeNode **)malloc(sizeof(BPlusTreeNode *) * LEVEL);
        if (node->keys == NULL || node->childs == NULL) {
            if (node->keys != NULL) {
                free(node->keys);
            }
            if (node->childs != NULL) {
                free(node->childs);
            }
            free(node);
            return NULL;
        }
        node->isRoot = (type == RootNode);
        node->isLeaf = false;
    }
    TotalNodes++;
    return node;
}

// static inline uint64_t BinarySearch(BPlusTreeNode *curNode, uint64_t key) {
//     uint64_t l = 0, r = curNode->keyNum;
//     if (key < curNode->keys[l]) return l;
//     if (curNode->keys[r - 1] <= key) return r - 1;
//     while (l < r - 1) {
//         uint64_t mid = l + ((r - l) >> 1);
//         if (curNode->keys[mid] > key)
//             r = mid;
//         else
//             l = mid;
//     }
//     return l;
// }

/* Search the index of key in curNode */
static inline uint64_t BinarySearchKey(BPlusTreeNode *curNode, uint64_t key) {
    uint64_t l = 0, r = curNode->keyNum - 1;
    while (l < r) {
        uint64_t mid = l + ((r - l) >> 1);
        if (curNode->keys[mid] == key)
            break;
        else if (curNode->keys[mid] > key)
            r = mid;
        else
            l = mid;
    }
    return l;
}

/* Search the index of child in curNode */
static inline uint64_t BinarySearchNode(BPlusTreeNode *curNode, uint64_t key) {
    uint64_t l = 0, r = curNode->keyNum;
    if (key < curNode->keys[l]) return l;
    if (key >= curNode->keys[r - 1]) return r;  // remember that: childs is always one more than keys
    while (l < r) {
        uint64_t mid = l + ((r - l) >> 1);
        if (curNode->keys[mid] == key)
            break;
        else if (curNode->keys[mid] > key)
            r = mid;
        else
            l = mid;
    }
    return l;
}

/* Search a leaf node which contains the specified key */
static BPlusTreeNode *LeafNodeSearch(uint64_t key) {
    if (Root == NULL) {
        printf("Root of B+tree is null.\n");
        exit(EXIT_FAILURE);
    }
    BPlusTreeNode *curNode = Root;
    while (1) {
        if (curNode->isLeaf) {
            break;
        }
        if (key < curNode->keys[0]) {
            curNode = curNode->childs[0];
        } else {
            uint64_t i = BinarySearchNode(curNode, key);
            curNode    = curNode->childs[i];
        }
    }
    return curNode;
}

static inline void FreeNode(BPlusTreeNode *node) {
    if (node == NULL) {
        return;
    }
    if (node->childs != NULL) {
        free(node->childs);
    }
    if (node->keys != NULL) {
        free(node->keys);
    }
    if (node->values != NULL) {
        free(node->values);
    }
    free(node);
    node->parent = NULL;
    node->prev   = NULL;
    node->next   = NULL;
    node->childs = NULL;
    node->keys   = NULL;
    node->values = NULL;
    node         = NULL;
}

static void Destroy_Tree(BPlusTreeNode *root) {
    if (root == NULL) {
        return;
    }
    if (root->isLeaf) {
        FreeNode(root);
    } else {
        for (uint64_t i = 0; i <= root->keyNum; i++) {
            Destroy_Tree(root->childs[i]);
        }
    }
}

static void LinkAfter(BPlusTreeNode *pos, BPlusTreeNode *node) {
    if (pos->next != NULL) {
        node->next      = pos->next;
        pos->next->prev = node;
    }
    pos->next  = node;
    node->prev = pos;
}

/* Copy keys, values(if src is leaf), childs from src to dest, update dest->keyNum */
static inline void CopyRecords(BPlusTreeNode *src, BPlusTreeNode *dest, uint64_t from, uint64_t range) {
    uint64_t i;
    for (i = 0; i < range; i++) {
        dest->keys[i] = src->keys[i + from];
        if (src->isLeaf && dest->isLeaf) {
            dest->values[i] = src->values[i + from];
        } else {
            dest->childs[i]         = src->childs[i + from];
            dest->childs[i]->parent = dest;
        }
    }
    dest->keyNum = range;
}

/* Insert key-value into node only when its keyNum < MAX_RECORDS_PER_NODE */
static void InsertAtLeafNode(BPlusTreeNode *node, uint64_t key, uint64_t value) {
    uint64_t i;
    for (i = node->keyNum; i > 0 && node->keys[i - 1] > key; i--) {
        node->keys[i]   = node->keys[i - 1];
        node->values[i] = node->values[i - 1];
    }
    node->keys[i - 1]   = key;
    node->values[i - 1] = value;
    node->keyNum++;
}

static void InsertAtParentNode(BPlusTreeNode *node, uint64_t key, BPlusTreeNode *child) {
    BPlusTreeNode *parent = node->isRoot ? node : node->parent;
    uint64_t i;
    for (i = parent->keyNum; i > 0 && parent->keys[i - 1] > key; i--) {
        parent->keys[i]       = parent->keys[i - 1];
        parent->childs[i + 1] = parent->childs[i];
    }
    parent->keys[i - 1] = key;
    parent->childs[i]   = child;
    parent->keyNum++;
}

static void SplitThenInsert(BPlusTreeNode *node, uint64_t key, BPlusTreeNode *child) {
    if (node->isRoot) {  // node is both root and leaf
        Root            = New_BPlusTreeNode(RootNode);
        Root->keyNum    = 1;
        Root->keys[0]   = key;
        Root->childs[0] = node;
        Root->childs[1] = child;
        node->isRoot    = false;
        node->parent = child->parent = Root;
        return;
    } else {
        InsertAtParentNode(node, key, child);
        LinkAfter(node, child);
    }

    BPlusTreeNode *parent = node->parent;
    if (parent->keyNum < MAX_RECORDS_PER_NODE) {
        InsertAtParentNode(node, key, child);
    } else {
        BPlusTreeNode *buffer = CreateBuffer();
        CopyRecords(parent, buffer, 0, parent->keyNum);
        InsertAtParentNode(buffer, key, child);

        BPlusTreeNode *rChild = New_BPlusTreeNode(InternalNode);
        uint64_t mid          = buffer->keyNum >> 1;
        CopyRecords(buffer, rChild, mid, buffer->keyNum - mid);
        parent->keyNum -= (mid - 1);
        DestroyBuffer(buffer);

        SplitThenInsert(parent, rChild->keys[0], rChild);
    }
}

/*=======================================================================*/
/* Allow duplicated key-value pair in b+tree */
/**
 * Check node->keyNum after executing insert operation every time.
 * Conditions:
 *      Con1: leaf is non full ,insert key-value directly
 *      Con2: leaf is full, leaf->parent is not full, split leaf then insert key in its parent 
 *      Con3: leaf is full and leaf->parent is also full, split leaf->parent recursively
 */
// WRONG:
extern void BPlusTree_Insert(uint64_t key, uint64_t value) {
    BPlusTreeNode *leaf = LeafNodeSearch(key);
    if (leaf->keyNum < MAX_RECORDS_PER_NODE) {
        InsertAtLeafNode(leaf, key, value);
    } else {
        BPlusTreeNode *buffer = CreateBuffer();
        buffer->isLeaf        = true;
        CopyRecords(leaf, buffer, 0, leaf->keyNum);
        InsertAtLeafNode(buffer, key, value);

        BPlusTreeNode *rNode = New_BPlusTreeNode(LeafNode);
        uint64_t mid         = buffer->keyNum >> 1;
        CopyRecords(buffer, rNode, mid, buffer->keyNum - mid);
        leaf->keyNum -= (mid - 1);

        DestroyBuffer(buffer);
        SplitThenInsert(leaf, rNode->keys[0], rNode);
    }
}

extern uint64_t BPlusTree_Select(uint64_t key) {
    BPlusTreeNode *leaf = LeafNodeSearch(key);
    uint64_t i          = BinarySearchKey(leaf, key);
    if (leaf->keys[i] == key)
        return leaf->values[i];
    else {
        printf("Key = %ld doesn't exist in the B+Tree.\n", key);
        return -1;
    }
}

extern uint64_t *BPlusTree_Select_Range(uint64_t key, uint64_t range, uint64_t *length) {
    BPlusTreeNode *leaf = LeafNodeSearch(key);
    uint64_t i          = BinarySearchKey(leaf, key);
    if (leaf->keys[i] != key) {
        printf("Key = %ld doesn't exist in the B+Tree.\n", key);
    }
    uint64_t *array = (uint64_t *)malloc(sizeof(uint64_t) * range);
    if (array == NULL) {
        return NULL;
    }
    uint64_t j = 0, k = 0;
    while (1) {
        if (k > range || leaf == NULL) {
            break;
        }
        while (j + i < leaf->keyNum) {
            array[j] = leaf->values[j + i];
            j++;
        }
        k += j;
        leaf = leaf->next;
    }
    *length = k;
    return array;
}

extern void BPlusTree_Init() {
    BPlusTree_Destroy();
    Root = New_BPlusTreeNode(RootNode);
    if (Root != NULL) {
        Root->isLeaf = true;
        Root->values = (uint64_t *)malloc(sizeof(uint64_t) * MAX_RECORDS_PER_NODE);
        printf("B+Tree has been initialized success.\n");
    } else {
        printf("Fail to init the B+Tree.\n");
    }
}

extern void BPlusTree_Destroy() {
    if (Root != NULL) {
        Destroy_Tree(Root);
    }
    printf("\nB+Tree has been destroyed.\n");
}

extern void BPlusTree_PrintTree() {
    BPlusTreeNode *curNode = Root;
    if (curNode == NULL) {
        printf("B+tree is NULL.\n");
        return;
    }
    printf("\n");
    PrintAllNodes(curNode);
}
