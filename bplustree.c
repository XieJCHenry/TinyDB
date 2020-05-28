
#include "bplustree.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "bplustree_utils.h"

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

static inline uint64_t BinarySearch(BPlusTreeNode *curNode, uint64_t key) {
    uint64_t l = 0, r = curNode->keyNum;
    if (key < curNode->keys[l]) return l;
    if (curNode->keys[r - 1] <= key) return r - 1;
    while (l < r - 1) {
        uint64_t mid = l + ((r - l) >> 1);
        if (curNode->keys[mid] > key)
            r = mid;
        else
            l = mid;
    }
    return l;
}

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

static BPlusTreeNode *LeafNodeFind(uint64_t key) {
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

//static void LinkBefore(BPlusTreeNode *pos, BPlusTreeNode *node) {
//    if (pos->prev != NULL) {
//        node->prev      = pos->prev;
//        pos->prev->next = node;
//    }
//    node->next = pos;
//    pos->prev  = node;
//}

static void LinkAfter(BPlusTreeNode *pos, BPlusTreeNode *node) {
    if (pos->next != NULL) {
        node->next      = pos->next;
        pos->next->prev = node;
    }
    pos->next  = node;
    node->prev = pos;
}

static inline void CopyRecords(BPlusTreeNode *src, BPlusTreeNode *dest, uint64_t from, uint64_t range) {
    uint64_t i;
    for (i = 0; i < range; i++) {
        dest->keys[i] = src->keys[i + from];
        if (src->isLeaf) {
            dest->values[i] = src->values[i + from];
            continue;
        }
        dest->childs[i] = src->childs[i + from];
    }
}

static inline void LeafNodeInsert(BPlusTreeNode *node, uint64_t key, uint64_t value) {
    uint64_t i;
    for (i = node->keyNum; i > 0 && node->keys[i - 1] > key; i--) {
        node->keys[i]   = node->keys[i - 1];
        node->values[i] = node->values[i - 1];
    }
    node->keys[i]   = key;
    node->values[i] = value;
    node->keyNum++;
}

static void SplitNode(BPlusTreeNode *node) {
    uint64_t j = node->keyNum / 2;

    if (node->isRoot) {  // RootNode
        NodeType type         = node->isLeaf ? LeafNode : InternalNode;
        BPlusTreeNode *rChild = New_BPlusTreeNode(type);
        CopyRecords(node, rChild, j + 1, node->keyNum - j - 1);
        rChild->keyNum = j;
        node->keyNum -= j;

        BPlusTreeNode *nRoot = New_BPlusTreeNode(RootNode);
        nRoot->keys[0]       = rChild->keys[0];
        nRoot->childs[0]     = node;
        nRoot->childs[1]     = rChild;
        nRoot->keyNum++;
        node->parent   = nRoot;
        rChild->parent = nRoot;
        Root           = nRoot;
        node->isLeaf   = false;
        node->isRoot   = false;

    } else {
        // TODO
        BPlusTreeNode *nNode = New_BPlusTreeNode(LeafNode);
        CopyRecords(node, nNode, j + 1, node->keyNum - j - 1);
        node->keyNum -= j;
        nNode->keyNum = j;
        // if LeafNode
        if (node->isLeaf && !node->isRoot) {
            LinkAfter(node, nNode);
        }
        // Both LeafNode and InternalNode
        if (node->parent->keyNum == MAX_RECORDS_PER_NODE) {
            SplitNode(node->parent);
            node->parent->keyNum    = 1;
            node->parent->keys[0]   = nNode->keys[0];
            node->parent->childs[1] = nNode;
            nNode->parent           = node->parent;
        } else {
            // append key and child directly
            node->parent->keys[node->parent->keyNum] = nNode->keys[0];
            node->parent->keyNum++;
            node->parent->childs[node->parent->keyNum] = nNode;
            nNode->parent                              = node->parent;
        }
    }
}

/*
static uint64_t ModifyRecord(BPlusTreeNode* node,uint64_t key){
    uint64_t i, val;
    i = BinarySearchKey(node, key);
    if (i == -1) {
        printf("Key=%ld is not exist.\n", key);
        return -1;
    }
    val = node->values[i];
    for (; i < node->keyNum - 1; i++) {
        node->keys[i]   = node->keys[i + 1];
        node->values[i] = node->values[i + 1];
    }
    node->keyNum--;
    TotalNodes--;

    return val;
}
*/

/*=======================================================================*/
/* Allow duplicated key-value pair in b+tree */
extern void BPlusTree_Insert(uint64_t key, uint64_t value) {
    BPlusTreeNode *leaf = LeafNodeFind(key);
    if (leaf->keyNum == MAX_RECORDS_PER_NODE) {
        SplitNode(leaf);
    }
    // PrintAllNodes(Root);
    leaf = LeafNodeFind(key);
    LeafNodeInsert(leaf, key, value);
}

extern uint64_t BPlusTree_Select(uint64_t key) {
    BPlusTreeNode *leaf = LeafNodeFind(key);
    uint64_t i          = BinarySearchKey(leaf, key);
    if (leaf->keys[i] == key)
        return leaf->values[i];
    else {
        printf("Key = %ld doesn't exist in the B+Tree.\n", key);
        return -1;
    }
}

extern uint64_t *BPlusTree_Select_Range(uint64_t key, uint64_t range, uint64_t *length) {
    BPlusTreeNode *leaf = LeafNodeFind(key);
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

extern uint64_t BPlusTree_Modify(uint64_t key, uint64_t newValue) {
    BPlusTreeNode *leaf = LeafNodeFind(key);
    uint64_t i          = BinarySearchKey(leaf, key);
    if (leaf->keys[i] == key) {
        uint64_t v      = leaf->values[i];
        leaf->values[i] = newValue;
        return v;
    } else {
        printf("Key=%ld doesn't exist in the b+tree.\n", key);
        return -1;
    }
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
