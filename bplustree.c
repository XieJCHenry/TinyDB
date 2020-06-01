
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

/* Copy keys, values(if src is leaf), childs from src to dest */
static inline void CopyRecords(BPlusTreeNode *src, BPlusTreeNode *dest, uint64_t from, uint64_t range) {
    uint64_t i;
    for (i = 0; i < range; i++) {
        dest->keys[i] = src->keys[i + from];
        if (src->isLeaf) {
            dest->values[i] = src->values[i + from];
            continue;
        }
        dest->childs[i]               = src->childs[i + from];
        src->childs[i + from]->parent = dest;
    }
}

/* Insert a key-value in node, update node->keyNum */
static inline void LeafNodeInsert(BPlusTreeNode *node, uint64_t key, uint64_t value) {
    uint64_t i;
    for (i = node->keyNum; i > 0 && node->keys[i - 1] > key; i--) {
        node->keys[i] = node->keys[i - 1];
        if (node->isLeaf)
            node->values[i] = node->values[i - 1];
    }
    node->keys[i] = key;
    if (node->isLeaf)
        node->values[i] = value;
    node->keyNum++;
}

/* Insert key-value into node only when its keyNum < MAX_RECORDS_PER_NODE */
static inline void InsertAtLeafNode(BPlusTreeNode *node, uint64_t key, uint64_t value) {
    uint64_t i;
    for (i = node->keyNum; i > 0 && node->keys[i - 1] > key; i--) {
        node->keys[i]   = node->keys[i - 1];
        node->values[i] = node->values[i - 1];
    }
    node->keys[i]   = key;
    node->values[i] = value;
    node->keyNum++;
}

/* Insert child into parent, then update keyNum, then update the value of keyNum */
static inline void InsertAtParentNode(BPlusTreeNode *parent, BPlusTreeNode *child) {
    uint64_t key = child->keys[0];
    uint64_t i;
    for (i = parent->keyNum; i > 0 && parent->keys[i - 1] > key; i--) {
        parent->keys[i]       = parent->keys[i - 1];
        parent->childs[i + 1] = parent->childs[i];
    }
    parent->keys[i]       = key;
    parent->childs[i + 1] = child;
    child->parent         = parent;
    parent->keyNum++;
}

/* Split leaf node to leaf and nNode, distribute data, and update 'parent' field */
static void SplitLeafAndInsert(BPlusTreeNode *leaf, uint64_t key, uint64_t value) {
    BPlusTreeNode *buffer = CreateBuffer();
    buffer->isLeaf        = true;
    CopyRecords(leaf, buffer, 0, leaf->keyNum);
    buffer->keyNum = leaf->keyNum;
    InsertAtLeafNode(buffer, key, value);

    uint64_t mid         = buffer->keyNum >> 1;
    BPlusTreeNode *nNode = New_BPlusTreeNode(LeafNode);
    CopyRecords(buffer, nNode, mid, buffer->keyNum - mid);
    nNode->keyNum = mid;
    leaf->keyNum -= mid;
    nNode->parent = leaf->parent;

    LinkAfter(leaf, nNode);
    InsertAtParentNode(leaf->parent, nNode);

    DestroyBuffer(buffer);
}

static void SplitParentAndInsert(BPlusTreeNode *parent, BPlusTreeNode *child) {
    BPlusTreeNode *buffer = CreateBuffer();
    CopyRecords(parent, buffer, 0, parent->keyNum);
    if (child != NULL)
        InsertAtParentNode(buffer, child);

    // create rNode
    BPlusTreeNode *rNode = New_BPlusTreeNode(InternalNode);
    // copy data from buffer to rNode
    uint64_t j = buffer->keyNum >> 1;
    CopyRecords(buffer, rNode, j, buffer->keyNum - j);
    rNode->keyNum  = parent->keyNum - j;
    parent->keyNum = j;
    DestroyBuffer(buffer);

    if (parent->isRoot) {
        Root            = New_BPlusTreeNode(RootNode);
        Root->keyNum    = 1;
        Root->keys[0]   = parent->keys[0];
        Root->childs[0] = parent;
        Root->keys[1]   = rNode->keys[0];
        Root->childs[1] = rNode;
        parent->isRoot  = false;
        rNode->parent   = Root;
    } else {
        if (parent->parent->keyNum < MAX_RECORDS_PER_NODE) {
            InsertAtParentNode(parent->parent, rNode);
        } else {
            SplitParentAndInsert(parent->parent, child);
        }
    }
}


static void InsertImplement(BPlusTreeNode *node, uint64_t key, uint64_t value) {
    if (node->keyNum < MAX_RECORDS_PER_NODE) {
        InsertAtLeafNode(node, key, value);
    } else {
        // SplitNode(node, key, value);
        if (node->isRoot) {
            SplitParentAndInsert(node, NULL);
        } else if (node->parent->keyNum < MAX_RECORDS_PER_NODE) {
            SplitLeafAndInsert(node, key, value);
        } else {
            SplitParentAndInsert(node->parent, node);
        }
    }
}

/*=======================================================================*/
/* Allow duplicated key-value pair in b+tree */
/**
 * Conditions:
 *      Con1: leaf is non full ,insert key-value directly
 *      Con2: leaf is full, leaf->parent is not full, split leaf then insert key in its parent 
 *      Con3: leaf is full and leaf->parent is also full, split leaf->parent recursively
 * TODO: test
 */
extern void BPlusTree_Insert(uint64_t key, uint64_t value) {
    BPlusTreeNode *leaf = LeafNodeSearch(key);
    InsertImplement(leaf, key, value);
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
