
#include "bplustree.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #include "bplustree_utils.h"

// #define Test_BinarySearch
#define DEPRECATED

BPlusTreeNode *Root;

uint64_t TotalNodes;
uint64_t TotalRecords;
uint64_t TreeHeight;

/**
 * The reason why use calloc rather than malloc:
 * I use Valgrind to analyse the allocate and free operation of memory.
 * Valgrind will point out that New_BPlusTreeNode() function will create
 * an uninitialised value Warning. In fact, this warning is caused by malloc()
 * function which won't clean up existing data and reset all bytes to zero.
 * Thus, Valgrind warn that 'Uninitialised value was created by a heap allocation'.
 * A simple way to fix this is to use calloc() function.
 * 
 * @Date: 2020-6-4
 */
static BPlusTreeNode *New_BPlusTreeNode(NodeType type) {
    // #ifdef DEBUG_TEST
    //     printf("Current memory suage = %ld kB\n", sizeof(BPlusTreeNode) * TotalNodes / (1024));
    // #endif
    BPlusTreeNode *node = calloc(1, sizeof(BPlusTreeNode));
    if (node == NULL) {
        return NULL;
    }
    node->keyNum = 0;
    node->parent = NULL;
    node->next   = NULL;
    node->prev   = NULL;
    if (type == LeafNode) {
        node->keys   = calloc(ORDER, sizeof(uint64_t));
        node->values = calloc(ORDER, sizeof(uint64_t));
        if (node->keys == NULL || node->values == NULL) {
            //it is ok to free a null pointer multi times and a non-null pointer once.
            free(node->keys);
            free(node->values);
            free(node);
            return NULL;
        }
        node->childs = NULL;
        node->isLeaf = true;
        node->isRoot = false;
    } else if (type == InternalNode || type == RootNode) {
        node->keys   = calloc(ORDER, sizeof(uint64_t));
        node->childs = calloc((ORDER + 1), sizeof(BPlusTreeNode *));
        if (node->keys == NULL || node->childs == NULL) {
            free(node->keys);
            free(node->childs);
            free(node);
            return NULL;
        }
        node->isRoot = (type == RootNode);
        node->isLeaf = false;
    }
    TotalNodes++;
    return node;
}

static inline void FreeNode(BPlusTreeNode **node) {
    BPlusTreeNode *n = *node;
    free(n->keys);
    free(n->values);
    free(n->childs);
    free(n);
    // n->next   = NULL;
    // n->prev   = NULL;
    // n->parent = NULL;
    // n->keys   = NULL;
    // n->values = NULL;
    // n->childs = NULL;
    // n->keyNum = 0;
    // *node     = NULL;
    TotalNodes--;
}

static void Destroy_Tree(BPlusTreeNode *root) {
    if (root == NULL) {
        return;
    }
    if (root->keyNum > 0 && !root->isLeaf) {
        uint64_t i;
        for (i = 0; i <= root->keyNum; i++) {
            Destroy_Tree(root->childs[i]);
        }
    }
    FreeNode(&root);
}

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

/* Search the index of key in curNode */
static inline uint64_t BinarySearchKey(BPlusTreeNode *curNode, uint64_t key) {
#ifdef Test_BinarySearch
    uint64_t l = 0, r = curNode->keyNum - 1;
    if (key <= curNode->keys[0])
        return 0;
    if (key == curNode->keys[r])
        return r;
    while (l < r) {
        uint64_t mid = l + ((r - l) >> 1);
        if (curNode->keys[mid] == key)
            l = mid + 1;
        else if (curNode->keys[mid] < key)
            l = mid + 1;
        else if (curNode->keys[mid] > key)
            r = mid;
    }
    return l - 1;
#else
    uint64_t i;
    for (i = 0; i < curNode->keyNum; i++) {
        if (curNode->keys[i] >= key) {
            break;
        }
    }
    return (i == curNode->keyNum) ? -1 : i;
#endif
}

/* Search the index of child in curNode */
static inline uint64_t BinarySearchNode(BPlusTreeNode *curNode, uint64_t key) {
#ifdef Test_BinarySearch
    uint64_t l = 0, r = curNode->keyNum;
    if (key <= curNode->keys[l])
        return l;
    if (key >= curNode->keys[r - 1])
        return r;  // childs is always one more than keys
    while (l < r) {
        uint64_t mid = l + (r - l) / 2;
        if (curNode->keys[mid] == key) {
            l = mid + 1;
        } else if (curNode->keys[mid] < key) {
            l = mid + 1;
        } else if (curNode->keys[mid] > key) {
            r = mid;
        }
    }
    return l - 1;
#else
    uint64_t i;
    for (i = 0; i < curNode->keyNum; i++) {
        if (curNode->keys[i] > key) {
            break;
        }
    }
    return i;
#endif
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

static void LinkAfter(BPlusTreeNode *pos, BPlusTreeNode *node) {
    if (pos == NULL || node == NULL) {
        return;
    }
    if (pos->next != NULL) {
        node->next      = pos->next;
        pos->next->prev = node;
    }
    pos->next  = node;
    node->prev = pos;
}

/**
 * 
 * On the whole, the specified insert implementation of B+tree includes the following requirements and steps:
 * 1. Requirements:
 * a. The key-value space allocated by each node is N, which is the order of B+tree.
 * b. Each node holds a pointer to its parent
 * 2. Steps:
 * a. Check the keyNum after an insert operation and determine whether to split.
 * b. I prefer iteration to update parent node from bottom to up.
 * @Date: 2020-6-3
 */
static void _BPlusTree_Insert(BPlusTreeNode *node, uint64_t key, uint64_t value) {
    // step1: directly insert
    uint64_t i;
    for (i = node->keyNum; i > 0 && node->keys[i - 1] > key; i--) {
        node->keys[i]   = node->keys[i - 1];
        node->values[i] = node->values[i - 1];
    }
    node->keys[i]   = key;
    node->values[i] = value;
    node->keyNum++;
    TotalRecords++;

    // step2: check if the node->keyNum > MAX_RECORDS_PER_NODE
    BPlusTreeNode *rNode = NULL, *parent = NULL;
    uint64_t temp, mid                   = -1;
    while (node->keyNum > MAX_RECORDS_PER_NODE) {
        // step2.1: split node into node and rNode, and divide the elements of node equally
        NodeType type = node->isLeaf ? LeafNode : InternalNode;
        rNode         = New_BPlusTreeNode(type);
        if (rNode == NULL) {
            return;
        }

        mid  = node->keyNum >> 1;  // node->keyNum = MAX_RECORDS_PER_NODE + 1 right now
        temp = node->keys[mid];
        if (node->isLeaf) {
            rNode->keyNum = node->keyNum - mid;
            memcpy(rNode->keys, node->keys + mid, sizeof(uint64_t) * (rNode->keyNum));
            memcpy(rNode->values, node->values + mid, sizeof(uint64_t) * (rNode->keyNum));
            LinkAfter(node, rNode);
        } else {
            rNode->keyNum = node->keyNum - mid - 1;
            memcpy(rNode->keys, node->keys + mid + 1, sizeof(uint64_t) * (rNode->keyNum));
            memcpy(rNode->childs, node->childs + mid + 1, sizeof(BPlusTreeNode *) * (node->keyNum - mid));
            for (i = 0; i <= rNode->keyNum; i++) {
                rNode->childs[i]->parent = rNode;
            }
        }
        node->keyNum = mid;

        // step2.2: judge if the node is Root
        parent = node->parent;
        if (node->isRoot) {  // node is root
            Root = New_BPlusTreeNode(RootNode);
            if (Root == NULL) {
                perror("Failed to allocate memory for Root.\n");
                exit(EXIT_FAILURE);
            }
            Root->childs[0] = node;
            node->parent    = Root;
            rNode->parent   = Root;
            node->isRoot    = false;
            parent          = Root;
            TreeHeight++;
        }
        // step2.3: update the right keys and childs of parent
        for (i = parent->keyNum; i > 0 && parent->keys[i - 1] > temp; i--) {
            parent->keys[i]       = parent->keys[i - 1];
            parent->childs[i + 1] = parent->childs[i];
        }
        parent->keys[i]       = temp;
        parent->childs[i + 1] = rNode;
        parent->keyNum++;

        rNode->parent = parent;
        node          = parent;  // bottom-up update
    }
}

/**
 * 
 * Performance Test:
 * Order = 1000
 * 1. isnert 10,000,000 records takes 21.510000 seconds
 * 2. insert 20,000,000 records takes 49.360000 seconds
 * 
 */
extern void BPlusTree_Insert(uint64_t key, uint64_t value) {
    // #ifdef DEBUG_TEST
    //     printf("enter BPlusTree_Insert()\n");
    // #endif
    if (Root == NULL) {
        BPlusTree_Init();
    }
    BPlusTreeNode *node = LeafNodeSearch(key);
    _BPlusTree_Insert(node, key, value);
}

/*=======================================================================*/

extern uint64_t BPlusTree_Select(uint64_t key) {
    uint64_t i          = -1;
    BPlusTreeNode *leaf = LeafNodeSearch(key);
    i                   = BinarySearchKey(leaf, key);
    if (leaf->keys[i] == key)
        return leaf->values[i];
    else {
        printf("Key = %ld doesn't exist in the B+Tree.\n", key);
        return -1;
    }
}

extern uint64_t *BPlusTree_Select_Range(uint64_t key, uint64_t range, uint64_t *length) {
    uint64_t i          = -1;
    BPlusTreeNode *leaf = LeafNodeSearch(key);
    i                   = BinarySearchKey(leaf, key);
    if (leaf->keys[i] != key) {
        printf("Key = %ld doesn't exist in the B+Tree.\n", key);
    }
    uint64_t *array = calloc(range, sizeof(uint64_t));
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

extern uint64_t BPlusTree_Update(uint64_t key, uint64_t newValue) {
    uint64_t oldValue, index = -1;
    BPlusTreeNode *leaf = LeafNodeSearch(key);
    index               = BinarySearchKey(leaf, key);
    if (leaf != NULL && index != -1) {
        oldValue            = leaf->values[index];
        leaf->values[index] = newValue;
    }
    return oldValue;
}

#ifndef DEPRECATED
/*===========================================*/
/**
 * borrow the last key-value from the left sibling of node
 * 
 */
static void Node_Right_Roate(BPlusTreeNode *node, uint64_t index) {
#ifdef DEBUG_TEST
    printf("Enter [Node_Left_Roate]\n");
#endif
    BPlusTreeNode *lSibling = node->prev, *parent = node->parent;
    uint64_t temp;
    temp = node->keys[0];
    memmove(node->keys + 1, node->keys, sizeof(uint64_t) * (node->keyNum));
    memmove(node->values + 1, node->values, sizeof(uint64_t) * (node->keyNum));
    node->keys[0]   = lSibling->keys[lSibling->keyNum - 1];
    node->values[0] = lSibling->values[lSibling->keyNum - 1];
    node->keyNum++;
    lSibling->keyNum--;
    /**
     * 如果index==0，且parent->keys[0] = temp，则替换parent->keys[0]。
     * 如果parent->keys[0] < temp，则说明leaf->keys[0]不会出现在parent->keys中。 
     */
    if (index == 0) {
        uint64_t idx = BinarySearchKey(parent, temp);
        if (idx == 0 && parent->keys[0] == temp)
            parent->keys[idx] = node->keys[0];
    }
}

/**
 * borrow the first key-value from the right sibling of node
 */
static void Node_Left_Roate(BPlusTreeNode *node) {
#ifdef DEBUG_TEST
    printf("Enter [Node_Right_Roate]\n");
#endif
    BPlusTreeNode *rSibling = node->next, *parent = node->parent;
    node->keys[node->keyNum] = rSibling->keys[0];
    node->keys[node->keyNum] = rSibling->values[0];
    node->keyNum++;
    // 更新parent中的key
    uint64_t idx      = BinarySearchKey(parent, rSibling->keys[0]);
    parent->keys[idx] = rSibling->keys[1];
    // 移动rSibling中的key-value
    memmove(rSibling->keys, rSibling->keys + 1, sizeof(uint64_t) * (rSibling->keyNum - 1));
    memmove(rSibling->values, rSibling->values + 1, sizeof(uint64_t) * (rSibling->keyNum - 1));
    rSibling->keyNum--;
}

/**
 * Merge node and its left sibling,both of them has the same parent.
 * 
 */
static void Merge_Left_Silbing(BPlusTreeNode *node, uint64_t key) {
#ifdef DEBUG_TEST
    printf("Enter [Merge_Left_Silbing]\n");
#endif
    BPlusTreeNode *lSilbing = node->prev, *nNext = node->next;
    uint64_t temp = node->keys[0];
    // 将node的key-value全部复制到lSilbing
    memcpy(lSilbing->keys + lSilbing->keyNum, node->keys, sizeof(uint64_t) * node->keyNum);
    memcpy(lSilbing->values + lSilbing->keyNum, node->values, sizeof(uint64_t) * node->keyNum);
    lSilbing->keyNum += node->keyNum;
    // 将key从parent中删除
    uint64_t i, index;
    index = BinarySearchKey(lSilbing->parent, temp);
    for (i = index; i < lSilbing->parent->keyNum - 1; i++) {
        lSilbing->parent->keys[i]       = lSilbing->parent->keys[i + 1];
        lSilbing->parent->childs[i + 1] = lSilbing->parent->childs[i + 2];
    }
    lSilbing->parent->childs[lSilbing->parent->keyNum] = NULL;
    lSilbing->parent->keyNum--;
    LinkAfter(lSilbing, nNext);
    node->next   = NULL;
    node->prev   = NULL;
    node->parent = NULL;
    FreeNode(&node);
}

static void Merge_Right_Silbing(BPlusTreeNode *node, uint64_t key) {
#ifdef DEBUG_TEST
    printf("Enter [Merge_Right_Silbing]\n");
#endif
    BPlusTreeNode *rSilbing = node->next, *parent = node->parent;
    uint64_t temp = rSilbing->keys[0];
    // 为了减少复制次数，将rSilbing的key-value全部复制到node中
    memcpy(node->keys + node->keyNum, rSilbing->keys, sizeof(uint64_t) * rSilbing->keyNum);
    memcpy(node->values + node->keyNum, rSilbing->values, sizeof(uint64_t) * rSilbing->keyNum);
    node->keyNum += rSilbing->keyNum;
    // 将temp从parent中删除
    uint64_t i, index;
    index = BinarySearchKey(parent, temp);
    for (i = index; i < parent->keyNum; i++) {
        parent->keys[i]       = parent->keys[i + 1];
        parent->childs[i + 1] = parent->childs[i + 2];
    }
    parent->keyNum--;
    parent->childs[parent->keyNum] = NULL;
    LinkAfter(node, rSilbing->next);
    rSilbing->next   = NULL;
    rSilbing->prev   = NULL;
    rSilbing->parent = NULL;
    FreeNode(&rSilbing);
}

#endif

// _leaf_node_left_rotate
// args: parent , 0
static void _Leaf_Node_Left_Rotate(BPlusTreeNode *parent, uint64_t index) {
#ifdef DEBUG_TEST
    printf("Enter _Leaf_Node_Left_Rotate()\n");
#endif
    BPlusTreeNode *node = parent->childs[index], *rSilbling = parent->childs[index + 1];
    node->keys[node->keyNum]   = rSilbling->keys[0];
    node->values[node->keyNum] = rSilbling->values[0];
    node->keyNum++;
    parent->keys[index] = rSilbling->keys[1];
    memmove(rSilbling->keys, rSilbling->keys + 1, sizeof(uint64_t) * (rSilbling->keyNum - 1));
    memmove(rSilbling->values, rSilbling->values + 1, sizeof(uint64_t) * (rSilbling->keyNum - 1));
    rSilbling->keyNum--;
}

// _internal_node_left_rotate
// args: parent , 0
static void _Internal_Node_Left_Rotate(BPlusTreeNode *parent, uint64_t index) {
#ifdef DEBUG_TEST
    printf("Enter _Internal_Node_Left_Rotate()\n");
#endif
    BPlusTreeNode *node = parent->childs[index], *rSilbling = parent->childs[index + 1];
    node->keys[node->keyNum]       = parent->keys[index];
    node->childs[node->keyNum + 1] = rSilbling->childs[0];
    node->keyNum++;
    parent->keys[index] = rSilbling->keys[0];
    memmove(rSilbling->keys, rSilbling->keys + 1, sizeof(uint64_t) * rSilbling->keyNum - 1);
    memmove(rSilbling->childs, rSilbling->childs + 1, sizeof(BPlusTreeNode *) * rSilbling->keyNum);
    rSilbling->keyNum--;
}

// _leaf_node_right_rotate
// args: parent , index - 1
static void _Leaf_Node_Right_Rotate(BPlusTreeNode *parent, uint64_t index) {
#ifdef DEBUG_TEST
    printf("Enter _Leaf_Node_Right_Rotate()\n");
#endif
    // BUG:这里必须是parent->childs[index+1]，理由暂不明确
    // 初步判定是链表链接出错
    BPlusTreeNode *node = parent->childs[index], *rSilbling = parent->childs[index + 1];
    memmove(rSilbling->keys + 1, rSilbling->keys, sizeof(uint64_t) * (rSilbling->keyNum));
    memmove(rSilbling->values + 1, rSilbling->values, sizeof(uint64_t) * (rSilbling->keyNum));
    rSilbling->keys[0]   = node->keys[node->keyNum - 1];
    rSilbling->values[0] = node->values[node->keyNum - 1];
    rSilbling->keyNum++;
    node->keyNum--;
    parent->keys[index] = rSilbling->keys[0];
}

// _internal_node_right_rotate
// args: parent , index - 1
static void _Internal_Node_Right_Rotate(BPlusTreeNode *parent, uint64_t index) {
#ifdef DEBUG_TEST
    printf("Enter _Internal_Node_Right_Rotate()\n");
#endif
    BPlusTreeNode *node = parent->childs[index], *rSilbling = parent->childs[index + 1];
    memmove(rSilbling->keys + 1, rSilbling->keys, sizeof(uint64_t) * rSilbling->keyNum);
    memmove(rSilbling->childs + 1, rSilbling->childs, sizeof(BPlusTreeNode *) * (rSilbling->keyNum + 1));
    rSilbling->keys[0]   = parent->keys[index];
    rSilbling->childs[0] = node->childs[node->keyNum];
    rSilbling->keyNum++;

    parent->keys[index] = node->keys[node->keyNum - 1];
    node->keyNum--;
}

// _node_merge_silbing
// args: parent , 0
// args: parent , index - 1
static void _Node_Merge_Silbing(BPlusTreeNode *parent, uint64_t index) {
#ifdef DEBUG_TEST
    printf("Enter _Node_Merge_Silbing()\n");
#endif
    BPlusTreeNode *node = parent->childs[index], *rSilbling = parent->childs[index + 1];

    if (node->isLeaf) {
        memcpy(node->keys + node->keyNum, rSilbling->keys, sizeof(uint64_t) * rSilbling->keyNum);
        memcpy(node->values + node->keyNum, rSilbling->values, sizeof(uint64_t) * rSilbling->keyNum);
        node->keyNum += rSilbling->keyNum;
    } else {
        node->keys[node->keyNum] = parent->keys[index];
        node->keyNum++;
        memcpy(node->keys + node->keyNum, rSilbling->keys, sizeof(uint64_t) * rSilbling->keyNum);
        uint64_t i;
        for (i = 0; i <= rSilbling->keyNum; i++) {
            rSilbling->childs[i]->parent   = node;
            node->childs[node->keyNum + i] = rSilbling->childs[i];
        }
        node->keyNum += rSilbling->keyNum;
    }

    // 修改父结点
    memmove(parent->keys + index, parent->keys + index + 1, sizeof(uint64_t) * (parent->keyNum - index - 1));
    memmove(parent->childs + index + 1, parent->childs + index + 2, sizeof(BPlusTreeNode *) * (parent->keyNum - index));
    parent->keyNum--;

    FreeNode(&rSilbling);
}

/*===========================================*/
/**
 * @param node: node that contains key
 * @param key: will be deleted
 * @param index: index of key at this node
 */
static void _BPlusTree_Delete(BPlusTreeNode *node, uint64_t key, uint64_t index) {
    uint64_t half;
    half = MAX_RECORDS_PER_NODE >> 1;
    memmove(node->keys + index, node->keys + index + 1, sizeof(uint64_t) * node->keyNum - 1);
    memmove(node->values + index, node->values + index + 1, sizeof(uint64_t) * node->keyNum - 1);
    node->keyNum--;
    if (index == 0 && !node->isRoot) {
        node->parent->keys[0] = node->keys[0];
    }

    BPlusTreeNode *curNode = node, *parent = node->parent, *silbing = NULL;
    while (curNode->keyNum < half && !curNode->isRoot) {
        uint64_t idxAtParent;
        for (idxAtParent = 0; idxAtParent <= parent->keyNum && curNode != parent->childs[idxAtParent]; idxAtParent++)
            ;
        if (idxAtParent > parent->keyNum) {
            printf("Error: Don't find node(%p) in parent(%p)\n", curNode, parent);
            return;
        }

        if (idxAtParent == 0) {
            /**
             * curNode 在 parent 中是第0个节点，则其 silbing 是第1个节点
             */
            silbing = parent->childs[1];
            if (silbing->keyNum > half) {
                if (curNode->isLeaf) {
                    _Leaf_Node_Left_Rotate(parent, 0);
                } else {
                    _Internal_Node_Left_Rotate(parent, 0);
                }
            } else {
                _Node_Merge_Silbing(parent, 0);
            }
        } else {
            /**
             * curNode 在 parent 中是第index个节点，其兄弟节点是第index - 1个子节点
             */
            silbing = parent->childs[idxAtParent - 1];
            if (silbing->keyNum > half) {
                if (curNode->isLeaf) {
                    _Leaf_Node_Right_Rotate(parent, idxAtParent - 1);
                } else {
                    _Internal_Node_Right_Rotate(parent, idxAtParent - 1);
                }
            } else {
                _Node_Merge_Silbing(parent, idxAtParent - 1);
            }
        }
        curNode = parent;
        parent  = curNode->parent;
    }

    // 单独处理根节点合并的情况
    if (curNode->isRoot) {
        if (curNode->keyNum == 0) {
            if (!curNode->isLeaf) {
                Root                       = curNode->childs[0];
                curNode->childs[0]->isRoot = true;
                FreeNode(&curNode);
            }
            TreeHeight--;
        }
    }
}

extern uint64_t BPlusTree_Delete(uint64_t key) {
    uint64_t value, index = -1;
    BPlusTreeNode *leaf = LeafNodeSearch(key);
    index               = BinarySearchKey(leaf, key);
    if (leaf != NULL && index != -1) {
        value = leaf->values[index];
    }
    _BPlusTree_Delete(leaf, key, index);

    return value;
}

extern void BPlusTree_Init() {
    BPlusTree_Destroy();
    Root = New_BPlusTreeNode(RootNode);
    if (Root != NULL) {
        Root->isLeaf = true;
        Root->values = calloc(ORDER, sizeof(uint64_t));
        TreeHeight   = 1;
        if (Root->values == NULL) {
            FreeNode(&Root);
            printf("Fail to init the B+Tree.\n");
        } else {
            printf("B+Tree has been initialized success.\n");
        }
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
    printf("\n");
}

extern uint64_t BPlusTree_AllRecords() {
    return TotalRecords;
}

extern uint64_t BPlusTree_AllNodes() {
    return TotalNodes;
}

extern uint64_t BPlusTree_Height() {
    return TreeHeight;
}