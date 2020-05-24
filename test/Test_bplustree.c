#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>

#include "../bplustree.h"
#include <stdlib.h>

#define PRINT_ATTRIBUTE(Struct, attribute) (printf(#Struct "->" #attribute "= %s\n", (char *)(Struct->attribute)))

int init_suite(void) {
    return 0;
}

int clean_suite(void) {
    return 0;
}

BPlusTreeNode *New_BPlusTreeNode(NodeType type) {
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
    // TotalNodes++;
    return node;
}

void PrintNode(BPlusTreeNode *node) {
    if (node == NULL) {
        printf("Node is NULL.\n");
        return;
    }
    PRINT_ATTRIBUTE(node, childs);
    // PRINT_ATTRIBUTE(node, isRoot);
    // PRINT_ATTRIBUTE(node, isLeaf);
    printf("node->isRoot = %d\n", node->isRoot);
    printf("node->isLeaf = %d\n", node->isLeaf);
    PRINT_ATTRIBUTE(node, keys);
    PRINT_ATTRIBUTE(node, values);
    CU_ASSERT_EQUAL(node->keys, NULL);
    CU_ASSERT_EQUAL(node->values, NULL);
}

void FreeNode(BPlusTreeNode *node) {
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
}

void test_New_BPlusTreeNode(void) {
    BPlusTreeNode *root = New_BPlusTreeNode(LeafNode);
    PrintNode(root);
    FreeNode(root);
    BPlusTreeNode *root1 = New_BPlusTreeNode(RootNode);
    PrintNode(root1);
    FreeNode(root1);
    BPlusTreeNode *root2 = New_BPlusTreeNode(InternalNode);
    PrintNode(root2);
    FreeNode(root2);
}

int main(void) {
    CU_pSuite pSuite = NULL;
    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }

    pSuite = CU_add_suite("Basic_Test_Suite", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    if ((NULL == CU_add_test(pSuite, "\n\n......Testing New_BPlusTreeNode function......\n\n", test_New_BPlusTreeNode))) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
}