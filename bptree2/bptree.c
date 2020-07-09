#include "./bptree.h"

#include <alloca.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../includes/file.h"
#include "../includes/global.h"

/*========================================*/
const uint64_t DEFAULT_PAGE_SIZE       = 4096;
const uint64_t BPTREE_NODE_HEADER_SIZE = TYPE_SIZE + PARENT_SIZE + NEXT_SIZE + PREV_SIZE + NUM_SIZE;
const uint64_t DEFAULT_ORDER           = DEFAULT_PAGE_SIZE / BPTREE_NODE_HEADER_SIZE;
const char *DEFAULT_INDEX_FILE         = "tinydb_index";
const char *DEFAULT_DATA_FILE          = "tinydb_data";
const char *DEFAULT_CONFIG_FILE        = "tinydb_config";
/*========================================*/

static BpTreeNode *New_BpTreeNode(BpTreeConfig *config);
static void Destroy_BpTreeNode(BpTreeNode *node);
static void DeserializeNode(BpTreeNode *node, void *buffer);
static void SerializeNode(BpTreeNode *node, void *buffer);

static uint64_t BpTreeNodeSearch(BpTreeNode *node, key_t key);

static BpTreeNode *New_BpTreeNode(BpTreeConfig *config) {
    uint64_t order   = config->order;
    BpTreeNode *node = (BpTreeNode *)calloc(1, sizeof(BpTreeNode) + order * sizeof(Index));
    assert(node != NULL);
    return node;
}
static void Destroy_BpTreeNode(BpTreeNode *node) {
    free(node);
}

/* 将buffer反序列化为node */
static inline void DeserializeNode(BpTreeNode *node, void *buffer) {
    memcpy(node, buffer, BPTREE_NODE_HEADER_SIZE);
    memcpy(node->childs, buffer + BPTREE_NODE_HEADER_SIZE, node->num * sizeof(Index));
}

/* 将node序列化为buffer */
static inline void SerializeNode(BpTreeNode *node, void *buffer) {
    memcpy(buffer, node, BPTREE_NODE_HEADER_SIZE);
    memcpy(buffer + BPTREE_NODE_HEADER_SIZE, node->childs, node->num * sizeof(Index));
}

static inline uint64_t BpTreeNodeSearch(BpTreeNode *node, key_t key) {
    uint64_t i;
    for (i = node->num - 1; i >= 0 && node->childs[i]->key > key; i--)
        ;
    return i;
}

/*========================================*/

BpTreeConfig *New_BpTreeConfig(uint64_t pageSize,
                               const char *indexFile,
                               const char *configFile,
                               const char *dataFile) {
    if (pageSize < 0 || pageSize % 2 != 0) {
        pageSize = DEFAULT_PAGE_SIZE;
    }
    if (indexFile == NULL || strlen(indexFile) <= 0) {
        indexFile = DEFAULT_INDEX_FILE;
    }
    if (configFile == NULL || strlen(configFile) <= 0) {
        configFile = DEFAULT_CONFIG_FILE;
    }
    if (dataFile == NULL || strlen(dataFile) <= 0) {
        dataFile = DEFAULT_DATA_FILE;
    }
    BpTreeConfig *config = (BpTreeConfig *)calloc(1, sizeof(BpTreeConfig));
    assert(config != NULL);
    config->pageSize      = pageSize;
    config->order         = pageSize / BPTREE_NODE_HEADER_SIZE;
    config->indexFileSize = DEFAULT_INDEX_FILE_INIT_SIZE;
    config->dataFileSize  = DEFUALT_DATA_FILE_INIT_SIZE;
    memcpy(config->configFile, configFile, strlen(configFile) + 1);
    memcpy(config->indexFile, indexFile, strlen(indexFile) + 1);
    memcpy(config->dataFile, dataFile, strlen(dataFile) + 1);
    return config;
}

/**
 * 如果传入的config为NULL，将会根据默认设定创建默认config
 * 
 */
BpTree *New_BpTree(BpTreeConfig *config) {
    BpTreeConfig *cfg = config;
    if (config == NULL) {
        cfg = New_BpTreeConfig(DEFAULT_PAGE_SIZE, DEFAULT_INDEX_FILE,
                               DEFAULT_CONFIG_FILE, DEFAULT_DATA_FILE);
    }
    BpTree *tree = (BpTree *)calloc(1, sizeof(BpTree));
    assert(tree != NULL);

    // step1: create file if not exists
    CreateFileIfNotExists(cfg->configFile, 1024);
    CreateFileIfNotExists(cfg->indexFile, cfg->indexFileSize);
    CreateFileIfNotExists(cfg->dataFile, cfg->dataFileSize);

    // step2: TODO: 将默认参数写入configFile

    // step3: 打开indexFile和dataFile
    tree->idxFd = OpenFile(cfg->indexFile);
    tree->datFd = OpenFile(cfg->dataFile);

    tree->height                = 0;
    tree->indexNum              = 0;
    tree->leafNum               = 0;
    tree->root                  = -1;
    tree->slot                  = 1;
    tree->config                = cfg;
    tree->config->dataFileSize  = FileLength(tree->datFd);
    tree->config->indexFileSize = FileLength(tree->idxFd);

    // step4.1: 初始化freeList
    FreeBlock *fblock = (FreeBlock *)calloc(1, sizeof(FreeBlock));
    assert(fblock != NULL);
    fblock->max_num    = cfg->indexFileSize / cfg->pageSize;
    fblock->num        = fblock->max_num;  // current num
    fblock->head       = New_FreeBlockNode();
    fblock->tail       = New_FreeBlockNode();
    fblock->head->next = fblock->tail;
    // flist->tail->next = NULL;

    // step4.2: 将indexFile文件内所有页面都加入到freeList中
    // uint64_t pageNum  = cfg->indexFileSize / cfg->pageSize;
    FreeBlockNode *ptr = fblock->head;
    uint64_t i;
    for (i = 0; i < fblock->max_num;) {
        ptr->curOffset  = i++;
        ptr->nextOffset = i;
        if (ptr->next == NULL) {
            ptr->next = New_FreeBlockNode();
        }
        ptr = ptr->next;
    }
    tree->freeBlock = fblock;

    return tree;
}

/**
 * 查找返回key在db文件中的偏移量
 * 
 * 从root开始，将结点从磁盘中读取到内存中，反序列化为内存结构，再查找键值对
 */
val_t BpTree_Select(BpTree *tree, key_t key) {
    val_t ret;
    BpTreeNode *root  = New_BpTreeNode(tree->config);
    uint64_t pageSize = tree->config->pageSize;
    void *buffer      = alloca(pageSize);
    int idxFd         = tree->idxFd;
    // if (lseek(idxFd, 0L, SEEK_SET) == -1) {
    //     perror("Seek Error.\n");
    // }
    // if (read(idxFd, buffer, pageSize) != pageSize) {
    //     perror("Fail to read a page.\n");
    // }
    S_SEEK(idxFd, 0L, SEEK_SET);
    S_READ(idxFd, buffer, pageSize);
    DeserializeNode(root, buffer);
    while (true) {
        // 如果是叶子结点，则代表记录的偏移量；如果是内部结点，代表子结点的偏移量
        uint64_t idx = BpTreeNodeSearch(root, key);
        if (root->type == Leaf) {
            ret = idx;
            break;
        } else {
            lseek(idxFd, idx, SEEK_SET);
            read(idxFd, buffer, pageSize);
            DeserializeNode(root, buffer);
        }
    }
    Destroy_BpTreeNode(root);
    return ret;
}

/**
 * 将key插入到B+树中，返回在db文件中待插入的位置
 */
val_t BpTree_Insert(BpTree *tree, key_t key) {
    BpTreeNode *root  = New_BpTreeNode(tree->config);
    uint64_t pageSize = tree->config->pageSize;
    void *buffer      = alloca(pageSize);
    int idxFd         = tree->idxFd;
   // if (lseek(idxFd, 0L, SEEK_SET) == -1) {
   //     perror("Seek Error.\n");
   // }
   // if (read(idxFd, buffer, pageSize) != pageSize) {
   //     perror("Fail to read a page.\n");
   // }
   // search node to insert
    S_SEEK(idxFd, 0L, SEEK_SET);
    S_READ(idxFd, buffer, pageSize);
    uint64_t idx;
    while(true){
	idx = BpTreeNodeSearch(root, key); // idx of leaf node
	if(root->type == Leaf) {
	    break;	
	} else {
	    S_SEEK(idxFd, idx, SEEK_SET);
	    S_READ(idxFd, buffer, pageSize);
	    DeserializeNode(root, buffer);
	}	    
   }
   // insert
   

}
