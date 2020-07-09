/**
 * B+ 树的职责：
 * 1. 提供B+树索引结构以供查找
 * 2. 维护B+树在文件中的结构
 * 
 * 从B+树执行查找后得到的结果是，记录在db文件中的偏移量。
 * 删除则为标记删除
 * 
 */

#ifndef BPTREE_BPTREE_H
#define BPTREE_BPTREE_H
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "../includes/global.h"

#define key_t uint64_t  // TODO: 暂时将 key_t 类型硬编码
#define val_t off_t     // TODO: 暂时将 val_t 类型硬编码
#define MAX_FILE_NAME_LENGTH 31
#define DEFUALT_DATA_FILE_INIT_SIZE (512 * 1024)
#define DEFAULT_INDEX_FILE_INIT_SIZE (512 * 1024)

typedef struct bptree_config_t BpTreeConfig;
typedef struct bptree_t BpTree;
typedef struct record_t Record;
typedef struct index_t Index;
typedef struct bptree_node_t BpTreeNode;
typedef struct free_block_t FreeBlock;

// struct record_t {
// };

/* 中存储记录的键和子结点偏移量 */
struct index_t {
    key_t key;
    val_t value;
};

typedef struct free_block_node_t FreeBlockNode;
struct free_block_node_t {
    uint64_t i;        // 初次分配时给予的编号
    off_t curOffset;   // 该空闲块在indexFile中的偏移量
    off_t nextOffset;  // 下一个空闲块在indexFile中的偏移量
    struct free_block_node_t *next;
};

static inline FreeBlockNode *New_FreeBlockNode() {
    FreeBlockNode *node = (FreeBlockNode *)calloc(1, sizeof(FreeBlockNode));
    assert(node != NULL);
    node->curOffset  = 0;
    node->nextOffset = 0;
    node->next       = NULL;
    node->i          = -1;
    return node;
}

struct free_block_t {
    uint64_t max_num;
    uint64_t num;
    struct free_block_node_t *head;
    struct free_block_node_t *tail;
};
static inline void FreeList_InsertAfter(FreeBlockNode *pos, FreeBlockNode *node) {
    node->next = pos->next;
    pos->next  = node;
}

typedef enum {
    Leaf     = 0,
    Root     = 1,
    Internal = 2
} BpTreeNodeType;

struct bptree_node_t {
    char type;
    off_t parent;
    off_t next;
    off_t prev;
    uint64_t num;
    struct index_t *childs[];
};
const size_t TYPE_SIZE   = SIZE_OF_ATTRIBUTE(BpTreeNode, type);
const size_t PARENT_SIZE = SIZE_OF_ATTRIBUTE(BpTreeNode, parent);
const size_t NEXT_SIZE   = SIZE_OF_ATTRIBUTE(BpTreeNode, next);
const size_t PREV_SIZE   = SIZE_OF_ATTRIBUTE(BpTreeNode, prev);
const size_t NUM_SIZE    = SIZE_OF_ATTRIBUTE(BpTreeNode, num);
extern const uint64_t BPTREE_NODE_HEADER_SIZE;

extern const uint64_t DEFAULT_PAGE_SIZE;
extern const uint64_t DEFAULT_ORDER;
extern const char *DEFAULT_INDEX_FILE;
extern const char *DEFAULT_DATA_FILE;
extern const char *DEFAULT_CONFIG_FILE;

struct bptree_config_t {
    uint64_t pageSize;  // 页面大小
    uint64_t order;     // B+树的阶
    uint64_t indexFileSize;
    uint64_t dataFileSize;
    char indexFile[MAX_FILE_NAME_LENGTH + 1];
    char configFile[MAX_FILE_NAME_LENGTH + 1];
    char dataFile[MAX_FILE_NAME_LENGTH + 1];
};

struct bptree_t {
    int idxFd;          // 索引文件的文件描述符
    int datFd;          // 数据文件的文件描述符
    uint64_t height;    // 当前树高（除去叶子层）
    uint64_t indexNum;  // 内部结点数量
    uint64_t leafNum;   // 叶子结点数量
    off_t root;         // 根结点在索引文件中的偏移量
    off_t slot;         // 下一个页面插入的位置
    BpTreeConfig *config;
    FreeBlock *freeBlock;
};

BpTreeConfig *New_BpTreeConfig(uint64_t pageSize,
                               const char *indexFile,
                               const char *configFile,
                               const char *dataFile);
BpTree *New_BpTree(BpTreeConfig *config);

val_t BpTree_Insert(BpTree *tree, key_t key);
val_t BpTree_Select(BpTree *tree, key_t key);

// void Init_BpTreeConfig(const char *cfgFile, BpTreeConfig *config);
// void Flush_BpTreeConfig(BpTreeConfig *config, int fd);

#endif