#ifndef PAGE_PAGER_CACHE2_H
#define PAGE_PAGER_CACHE2_H

#include <stdint.h>

#include "pager.h"

// #define PAGER_CACHE_MAX_BLOCK_SIZE 32

// #define ENTRY_RW_FLAG_0 0  // 00: 最近未被访问且未被修改
// #define ENTRY_RW_FLAG_1 1  // 01：最近未被访问但被修改
// #define ENTRY_RW_FLAG_2 2  // 10：最近被访问但未被修改
// #define ENTRY_RW_FLAG_3 3  // 11：最近被访问且被修改
// #define ENTRY_RW_MASK 7    // 111

// typedef struct pager_cache_t PagerCache;
// typedef struct pager_cache_node_t PagerCacheNode;

// struct pager_cache_t {
//     uint32_t usedNum;
//     uint32_t freeNum;
//     struct pager_cache_node_t *curPage;   // points to a page in usedList
//     struct pager_cache_node_t *usedList;  // cyclic queue
//     struct pager_cache_node_t *freeList;  // link stack
// };

// struct pager_cache_node_t {
//     uint8_t flag;
//     Page *page;
//     struct pager_cache_node_t *next;
// };

// TINYDB_API PagerCache *New_PagerCache();
// TINYDB_API void Destroy_PagerCache(PagerCache *pagerCache);
// TINYDB_API uint32_t PagerCache_Insert(PagerCache *cache, Record *record);
// TINYDB_API Page *PagerCache_Select(PagerCache *cache, KEY id);
// TINYDB_API void PagerCache_Update(PagerCache *cache);
// TINYDB_API void PagerCache_Delete(PagerCache *cache);
// TINYDB_API uint32_t PagerCache_Flush();  // return the number of pages that flushed to disk

#endif