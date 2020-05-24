
#include "./bplustree.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

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

static BPlusTreeNode *LeafNodeFind(uint64_t key) {
    BPlusTreeNode *curNode = Root;
    while (1) {
        if (curNode->isLeaf == true) {
            break;
        }
        if (key < curNode->keys[0]) {
            curNode = curNode->childs[0];
        } else {
            uint64_t i = BinarySearch(curNode, key);
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

static inline void LeafNodeInsert(BPlusTreeNode *node, uint64_t key, uint64_t value) {
    uint64_t i;
    for (i = node->keyNum; i > 0 && node->keys[i - 1] > key; i--) {
        node->keys[i]   = node->keys[i - 1];
        node->values[i] = node->values[i - 1];
    }
    node->keys[i]   = key;
    node->values[i] = value;
}

static void SplitNode(BPlusTreeNode *node){
    
}

/*=======================================================================*/
/* Allow duplicated key-value pair in b+tree */
extern void BPlusTree_Insert(uint64_t key, uint64_t value) {
    BPlusTreeNode *leaf = LeafNodeFind(key);
    uint64_t i          = BinarySearchKey(leaf, key);
    if (leaf->keyNum < MAX_RECORDS_PER_NODE) {
        // direct insert
        LeafNodeInsert(leaf, key, value);
    } else {
        // insert after splitting node
    }
}

extern uint64_t BPlusTree_Select(uint64_t key) {
    BPlusTreeNode *leaf = LeafNodeFind(key);
    uint64_t i          = BinarySearchKey(leaf, key);
    if (leaf->keys[i] == key)
        return leaf->values[i];
    else {
        printf("Key = %ld doesn't exist in the B+Tree.\n");
        return -1;
    }
}

extern uint64_t *BPlusTree_Select_Range(uint64_t key, uint64_t range, uint64_t *length) {
    BPlusTreeNode *leaf = LeafNodeFind(key);
    uint64_t i          = BinarySearchKey(leaf, key);
    if (leaf->keys[i] != key) {
        printf("Key = %ld doesn't exist in the B+Tree.\n");
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
        printf("Key= %ld doesn't exist in the b+tree.\n");
        return -1;
    }
}

extern void BPlusTree_Init() {
    BPlusTree_Destroy();
    Root = New_BPlusTreeNode(RootNode);
    if (Root != NULL) {
        Root->isLeaf = true;
        printf("B+Tree has been initialized success.\n");
    } else {
        printf("Fail to init the B+Tree.\n");
    }
}

extern void BPlusTree_Destroy() {
    if (Root != NULL) {
        Destroy_Tree(Root);
    }
    printf("B+Tree has been destroyed.\n");
}