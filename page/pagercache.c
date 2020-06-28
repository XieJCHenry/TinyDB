#include "./pagercache.h"

#include <stdlib.h>

#include "../includes/global.h"
// #include "./pager.h"

/*---------------------------- Private Function ----------------------------*/
/**
 * TODO: how to init the flags
 */
PRIVATE PagerCacheEntry *New_PagerCacheEntry() {
    PagerCacheEntry *entry = (PagerCacheEntry *)calloc(1, sizeof(PagerCacheEntry));
    if (entry == NULL) {
        EXIT_ERROR("Failed to allocate memory for PagerCacheEntry.\n");
    }
    entry->flags = ENTRY_RW_FLAG_0;
    entry->page  = NULL;
    entry->next  = NULL;
    return entry;
}

PRIVATE void Destroy_PagerCacheEntry(PagerCacheEntry *entry) {
    if (entry == NULL) {
        return;
    }
    if (entry->page != NULL) {
        free(entry->page);
    }
    entry->next = NULL;
    free(entry);
    entry = NULL;
}

PRIVATE void PagerCacheEntry_LinkAfter(PagerCacheEntry *pos, PagerCacheEntry *entry);
PRIVATE void PagerCacheEntry_LinkBefore(PagerCacheEntry *pos, PagerCacheEntry *entry);

/**
 * Unlink the curNode by copying data from its next node,
 * then insert it at the head of freeList.
 */
PRIVATE void PagerCacheEntry_UnLink(PagerCache *cache, PagerCacheEntry *prev, PagerCacheEntry *entry) {
    if (entry->next == NULL && prev != NULL) {
        prev->next = NULL;  // last entry in usedList
    } else if (prev == NULL) {
        cache->usedList = entry->next;  // first entry in usedList
    } else {
        prev->next = entry->next;  // at an inner position of usedList
    }
    // insert at the head of freeList
    entry->next     = cache->freeList;
    cache->freeList = entry;

    cache->usedNum--;
    cache->freeNum++;
}

PRIVATE uint32_t PagerCache_SearchPageIndex(PagerCache *cache, KEY id) {
    PagerCacheEntry *list;
    uint32_t i = 0;
    for (list = cache->usedList; list != NULL; list = list->next) {
        Page *page = list->page;
        if (page->rows[page->rowCount - 1]->id > id) {
            break;
        } else if (page->rows[page->rowCount - 1]->id == id) {
            perror("tinydb is not supported to insert duplicated id.\n");
            return -1;
        }
        i++;
    }
    return i;
}

// PRIVATE uint32_t Pager

PRIVATE void PagerCache_DestroyList(PagerCacheEntry **list) {
    PagerCacheEntry *prev, *next;
    prev = *list;
    while (prev != NULL) {
        next = prev->next;
        Destroy_PagerCacheEntry(prev);
        prev = next;
    }
    *list = NULL;
}

/*---------------------------- Public Api ----------------------------*/

TINYDB_API PagerCache *New_PagerCache() {
    PagerCache *cache = (PagerCache *)calloc(1, sizeof(PagerCache));
    if (cache == NULL) {
        EXIT_ERROR("Failed to allocate memory for PagerCache.\n");
    }
    cache->freeNum  = 0;
    cache->usedNum  = 0;
    cache->freeList = New_PagerCacheEntry();
    cache->usedList = New_PagerCacheEntry();
    cache->curPage  = NULL;
    return cache;
}

TINYDB_API void Destroy_PagerCache(PagerCache *pagerCache) {
    if (pagerCache == NULL) {
        return;
    }
    PagerCache_DestroyList(&(pagerCache->freeList));
    pagerCache->freeNum = 0;
    PagerCache_DestroyList(&(pagerCache->usedList));
    pagerCache->usedNum = 0;
    pagerCache->curPage = NULL;
    free(pagerCache);
    pagerCache = NULL;
}

/**
 * Search the correct page of the record to insert.
 * If the rowCount of a page reaches the upper limit, execute the replace strategy.
 */
TINYDB_API uint32_t PagerCache_Insert(PagerCache *cache, Record *record) {
    KEY id = record->id;
    uint32_t i, j;
    PagerCacheEntry *entry;
    entry = cache->usedList;
    // search cache
    for (i = 0; i < cache->usedNum; i++) {
        Page *page = entry->page;
        if (page->rows[page->rowCount - 1]->id > id) {
            for (j = page->rowCount; j > 0 && page->rows[j - 1]->id > id; j--) {
                page->rows[j] = page->rows[j - 1];
            }
            page->rows[j] = record;
            page->lastModifiedRow = j;
            page->rowCount++;
            page->lastRowOffset += PAGE_SIZE;
            page->lastModifiedTime = time(NULL);

            entry->flags = ENTRY_RW_FLAG_3;
        } else if (page->rows[page->rowCount - 1]->id == id) {
            perror("tinydb is not supported to insert duplicated id.\n");
            return 0;  // false
        }

        /**
         *  TODO: check rowCount after insert
         * 页面已满，如果采用溢出块或寻找空位的办法，依然需要在写回磁盘的时候对文件进行重整。
         * 即便采用：创建新pageEntry，将原来的其中一半拷贝到另一半的办法，在写回磁盘时也要重整磁盘。
         */
        // 当前entry并不是待插入的页面，TODO: 因此设置访问位为0。
        // 修改位只增不减，因此此处不设为0。
        entry = entry->next;
    }

    return 1;
}