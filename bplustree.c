#include "bplustree.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
        node->keys   = (uint64_t *)malloc(sizeof(uint64_t) * LEVEL);  // allocate one more space
        node->values = (uint64_t *)malloc(sizeof(uint64_t) * LEVEL);  // allocate one more space
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
        node->keys   = (uint64_t *)malloc(sizeof(uint64_t) * LEVEL);
        node->childs = (BPlusTreeNode **)malloc(sizeof(BPlusTreeNode *) * (LEVEL+1));
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
static BPlusTreeNode *LeafNodeSearch(uint64_t key, uint64_t *index) {
    if (Root == NULL) {
        printf("Root of B+tree is null.\n");
        exit(EXIT_FAILURE);
    }
    BPlusTreeNode *curNode = Root;
    *index                 = 0;
    while (1) {
        if (curNode->isLeaf) {
            break;
        }
        if (key < curNode->keys[0]) {
            curNode = curNode->childs[0];
        } else {
            uint64_t i = BinarySearchNode(curNode, key);
            curNode    = curNode->childs[i];
            *index     = i;
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

/* TODO: Error: Double Free */
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


/**
 * On the whole, the specified implementation of B+tree includes the following requirements and steps:
 * 1. Requirements:
 * a. The key-value space allocated by each node is N, which is the order of B+tree.
 * b. Each node holds a pointer to its parent
 * 2. Steps:
 * a. Check the keyNum after an insert operation and determine whether to split.
 * b. I prefer iteration to update parent node from bottom to up.
 * @Date: 2020-6-3
 */
static void _BPlusTree_Insert(BPlusTreeNode *node, uint64_t key, uint64_t value) {
    uint64_t i;
    for (i = node->keyNum; i > 0 && node->keys[i] > key; i--) {
        node->keys[i]   = node->keys[i - 1];
        node->values[i] = node->values[i - 1];
    }
    node->keys[i]   = key;
    node->values[i] = value;
    node->keyNum++;

    BPlusTreeNode *rNode = NULL, *parent = NULL;
    uint64_t temp, mid                   = -1;
    while (node->keyNum > MAX_RECORDS_PER_NODE) {
        NodeType type = node->isLeaf ? LeafNode : InternalNode;
        rNode         = New_BPlusTreeNode(type);
        assert(rNode != NULL);

        mid  = node->keyNum >> 1; // node->keyNum = MAX_RECORDS_PER_NODE + 1 right now
        temp = node->keys[mid];
        if (node->isLeaf) {
            rNode->keyNum = node->keyNum - mid;
            memcpy(rNode->keys, node->keys + mid, sizeof(uint64_t) * (rNode->keyNum));
            memcpy(rNode->values, node->values + mid, sizeof(uint64_t) * (rNode->keyNum));
            LinkAfter(node, rNode);
        } else {
            rNode->keyNum = node->keyNum - mid - 1;
            memcpy(rNode->keys, node->keys + mid + 1, sizeof(uint64_t) * (rNode->keyNum));
            memcpy(rNode->childs, node->childs + mid + 1, sizeof(BPlusTreeNode) * (node->keyNum - mid));
            for (i = 0; i <= rNode->keyNum; i++) {
                rNode->childs[i]->parent = rNode;
            }
        }
        node->keyNum = mid;

        parent = node->parent;
        if (parent == NULL) { // node is root
            Root            = New_BPlusTreeNode(RootNode);
            Root->childs[0] = node;
            node->parent    = Root;
            rNode->parent   = Root;
            node->isRoot    = false;
            parent          = Root;
            // parent->childs[0] = node;
            // node->parent      = parent;
            // rNode->parent     = parent;
        }
        // update the right keys and childs of parent
        for (i = parent->keyNum; i > 0 && parent->keys[i - 1] > temp; i--) {
            parent->keys[i]       = parent->keys[i - 1];
            parent->childs[i + 1] = parent->childs[i];
        }
        parent->keys[i]       = temp;
        parent->childs[i + 1] = rNode;
        parent->keyNum++;

        rNode->parent = parent;
        node          = parent; // bottom-up update
    }
}

/*=======================================================================*/
/* Do not Allow duplicated key-value pair in b+tree */
/**
 * Check node->keyNum after executing insert operation every time.
 * Conditions:
 *      Con1: leaf is non full ,insert key-value directly
 *      Con2: leaf is full, leaf->parent is not full, split leaf then insert key in its parent 
 *      Con3: leaf is full and leaf->parent is also full, split leaf->parent recursively
 */
extern void BPlusTree_Insert(uint64_t key, uint64_t value) {
    if (Root == NULL) {
        BPlusTree_Init();
    }
    uint64_t index      = -1;
    BPlusTreeNode *node = LeafNodeSearch(key, &index);
    if (index != -1) {
        if (node->values[index] == value) {
            printf("Key = %ld is already existed.\n", key);
            return;
        }
    }
    _BPlusTree_Insert(node, key, value);
}

/*=======================================================================*/

extern uint64_t BPlusTree_Select(uint64_t key) {
    uint64_t i          = -1;
    BPlusTreeNode *leaf = LeafNodeSearch(key, &i);
    if (leaf->keys[i] == key)
        return leaf->values[i];
    else {
        printf("Key = %ld doesn't exist in the B+Tree.\n", key);
        return -1;
    }
}

extern uint64_t *BPlusTree_Select_Range(uint64_t key, uint64_t range, uint64_t *length) {
    uint64_t i          = -1;
    BPlusTreeNode *leaf = LeafNodeSearch(key, &i);
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
