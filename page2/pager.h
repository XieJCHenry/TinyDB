#ifndef PAGE_PAGER2_H
#define PAGE_PAGER2_H

#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "../includes/global.h"

#define USERNAME_LENGTH 31
#define EMAIL_LENGTH 31
#define MAX_DB_FILE_LENGTH 255

#define KEY uint32_t
#define PAGE_SIZE 4096

#define SIZE_OF_ATTRIBUTE(struct, attr) (sizeof(((struct *)0)->attr))
#define ROW_BYTE_OFFSET(cursorIndex, pageSize, rowSize, rowsOffset) ((cursorIndex) / (pageSize) + (cursorIndex) * (rowSize) + (rowsOffset))

typedef struct record_t Record;
typedef struct page_t Page;
typedef struct page_entry_t PageEntry;
typedef struct pager_t Pager;
typedef struct table_t Table;
typedef struct cursor_t Cursor;

typedef enum {
    Pager_ExecuteFailed  = -1,
    Pager_ExecuteSuccess = 0,

    Pager_RowNotFound      = 1,
    Pager_RowAleardyExists = 2
} PagerExecuteResult;

struct record_t {
    KEY id;
    bool isOnline;
    char username[USERNAME_LENGTH + 1];
    char email[EMAIL_LENGTH + 1];
};
/*===================== Record =====================*/
const uint32_t ID_SIZE       = SIZE_OF_ATTRIBUTE(Record, id);
const uint32_t ONLINE_SIZE   = SIZE_OF_ATTRIBUTE(Record, isOnline);
const uint32_t USERNAME_SIZE = SIZE_OF_ATTRIBUTE(Record, username);
const uint32_t EMAIL_SIZE    = SIZE_OF_ATTRIBUTE(Record, email);
const uint32_t RECORD_SIZE   = ID_SIZE + ONLINE_SIZE + USERNAME_SIZE + EMAIL_SIZE;

const uint32_t ID_OFFSET       = 0;
const uint32_t ONLINE_OFFSET   = ID_OFFSET + ID_SIZE;
const uint32_t USERNAME_OFFSET = ONLINE_OFFSET + ONLINE_SIZE;
const uint32_t EMAIL_OFFSET    = USERNAME_OFFSET + USERNAME_SIZE;
/*==================================================*/

/*====================== Page ======================*/
const uint32_t ENTRY_NUM_SIZE           = SIZE_OF_ATTRIBUTE(Page, EntryNum);
const uint32_t LAST_READED_ENTRY_SIZE   = SIZE_OF_ATTRIBUTE(Page, lastReadedEntry);
const uint32_t LAST_MODIFIED_ENTRY_SIZE = SIZE_OF_ATTRIBUTE(Page, lastModifiedEntry);
const uint32_t LAST_ENTRY_OFFSET_SIZE   = SIZE_OF_ATTRIBUTE(Page, lastEntryOffset);
const uint32_t FIRST_DELETED_ENTRY_SIZE = SIZE_OF_ATTRIBUTE(Page, firstDeletedEntry);
const uint32_t LAST_READED_TIME_SIZE    = SIZE_OF_ATTRIBUTE(Page, lastReadedTime);
const uint32_t LAST_MODIFIED_TIME_SIZE  = SIZE_OF_ATTRIBUTE(Page, lastModifiedTime);
const uint32_t OFFSET_IN_FILE_SIZE      = SIZE_OF_ATTRIBUTE(Page, offsetInFile);
const uint32_t NEXT_OFFSET_IN_FILE_SIZE = SIZE_OF_ATTRIBUTE(Page, nextOffsetInFile);
const uint32_t PAGE_HEADER_SIZE         = ENTRY_NUM_SIZE + LAST_READED_ENTRY_SIZE + LAST_MODIFIED_ENTRY_SIZE + LAST_ENTRY_OFFSET_SIZE + FIRST_DELETED_ENTRY_SIZE + LAST_READED_TIME_SIZE + LAST_MODIFIED_TIME_SIZE + OFFSET_IN_FILE_SIZE + NEXT_OFFSET_IN_FILE_SIZE;
/*==================================================*/
const uint32_t ENTRY_SIZE          = RECORD_SIZE + SIZE_OF_ATTRIBUTE(PageEntry, nextEntry);
const uint32_t MAX_EntryS_PER_PAGE = (PAGE_SIZE - PAGE_HEADER_SIZE) / ENTRY_SIZE;

struct page_t {
    uint32_t entryNum;                                  // 记录数
    uint32_t blockEntryNum;                             // 溢出块记录数
    uint32_t lastReadedEntry;                           // 最后一次读取的记录
    uint32_t lastModifiedEntry;                         // 最后一次修改的记录
    uint32_t lastEntryOffset;                           // 最后一个记录的字节偏移量
    uint32_t firstDeletedEntry;                         // 第一个被删除记录的行标
    time_t lastReadedTime;                              // 最后一次读取的时间
    time_t lastModifiedTime;                            // 最后一次修改的时间
    uint32_t offsetInFile;                              // 该页面在文件内的序号（从0开始）
    uint32_t nextOffsetInFile;                          // 下一个页面在文件内的序号（从0开始）
    PageEntry *entries[MAX_EntryS_PER_PAGE];            // 记录
    PageEntry *overflowBlock[MAX_EntryS_PER_PAGE / 2];  // 溢出块
};

/**
 * It looks like a static linked list.
 */
struct page_entry_t {
    Record *record;
    uint32_t nextEntry;  // index of nextEntry in page, it use just for delete
};

struct pager_t {
    int fd;
    off_t fileSize;
    uint32_t pageNum;
    PagerCache *cache;
    char file[];
};

/* table 将所有操作都交给pager执行，TODO:后续要将table从pager.h中移出，因为这是一个更高层次的概念，不在该层调用 */
struct table_t {
    uint32_t rowCount;
    Pager *pager;
    Cursor *cursor;
};

struct cursor_t {
    uint32_t index;
};

TINYDB_API Pager *New_Pager(const char *file);
TINYDB_API void Destroy_Pager(Pager *pager);
TINYDB_API PagerExecuteResult Pager_Insert(Pager *pager, Record *record);
TINYDB_API PagerExecuteResult Pager_Select(Pager *pager, Cursor *cursor, KEY id, Record **ret);
TINYDB_API PagerExecuteResult Pager_Update(Pager *pager, Cursor *cursor, Record *record, Record **ret);
TINYDB_API PagerExecuteResult Pager_Delete(Pager *pager, Cursor *cursor, Record **ret);


/* ===================== PagerCache ===================== */


#define PAGER_CACHE_MAX_BLOCK_SIZE 32

#define ENTRY_RW_FLAG_0 0  // 00: 最近未被访问且未被修改
#define ENTRY_RW_FLAG_1 1  // 01：最近未被访问但被修改
#define ENTRY_RW_FLAG_2 2  // 10：最近被访问但未被修改
#define ENTRY_RW_FLAG_3 3  // 11：最近被访问且被修改
#define ENTRY_RW_MASK 7    // 111

typedef struct pager_cache_t PagerCache;
typedef struct pager_cache_node_t PagerCacheNode;

struct pager_cache_t {
    uint32_t usedNum;
    uint32_t freeNum;
    struct pager_cache_node_t *curPage;   // points to a page in usedList
    struct pager_cache_node_t *usedList;  // cyclic queue
    struct pager_cache_node_t *freeList;  // link stack
};

struct pager_cache_node_t {
    uint8_t flag;
    Page *page;
    struct pager_cache_node_t *next;
};

TINYDB_API PagerCache *New_PagerCache();
TINYDB_API void Destroy_PagerCache(PagerCache *pagerCache);
TINYDB_API uint32_t PagerCache_Insert(PagerCache *cache, Record *record);
TINYDB_API Page *PagerCache_Select(PagerCache *cache, KEY id);
TINYDB_API void PagerCache_Update(PagerCache *cache);
TINYDB_API void PagerCache_Delete(PagerCache *cache);
TINYDB_API uint32_t PagerCache_Flush();  // return the number of pages that flushed to disk

#endif