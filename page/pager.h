#ifndef PAGE_PAGER_H
#define PAGE_PAGER_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <time.h>

#include "../includes/global.h"

/* use g++ compiler */
#define USE_GPLUSPLUS_COMPILE

#define SIZE_OF_ATTRIBUTE(struct, attr) (sizeof(((struct *)0)->attr))
#define OFFSET_OF_ATTRIBUTE(struct, attr) ((size_t) & (((struct *)0)->attr))

#define USERNAME_LENGTH 31
#define EMAIL_LENGTH 31
#define MAX_DB_FILE_LENGTH 255

#define TABLE_MAX_PAGES 100
#define PAGE_SIZE 4096

#define KEY int32_t

typedef enum {
    Pager_ExecuteFailed  = -1,
    Pager_ExecuteSuccess = 0,

    Pager_RowNotFound      = 1,
    Pager_RowAleardyExists = 2
} PagerExecuteResult;

/**
 * db文件的文件头，记录元数据，元数据存储在另一个文件中，与数据文件分开。
 * ----------------------------------------
 * 标记被删除记录以免去移动数据的麻烦：
 * 文件头存访第一个被删除记录的偏移量。
 * 清空每一个被删除记录的内容，然后将当前位置的偏移量存访到上一个删除位置的偏移量里，
 * 最后一个被删除记录存放的偏移量为-1，说明已到达最后一条被删除记录。
 * 考虑插入的情况：
 * 1、如果插入位置恰好为被删除位置，则直接插入记录，并更新前一个删除位所记录的偏移量。（即链表删除操作）
 * 2、如果插入位置不是被删除位，搜索下一个删除标记位：
 *      2.1 如果插入位置不是位于最后一个删除标记位之后，那么就将[待插入位置，下一个删除标记位]范围内的
 *          所有记录都后移RowSize的位置（复制到内存缓冲区），插入新记录。最后更新上一个删除标记位的值。（即链表删除操作）
 *      2.2 如果插入位置是最后一个删除标记位之后，将[带插入位置，文件末尾]范围内的所有记录都复制到文件缓冲区，
 *          插入新记录，然后将文件缓冲区的内容写回到文件中。
 * ----------------------------------------
 */

typedef struct Row {
    KEY id;
    bool isOnline;
    char username[USERNAME_LENGTH + 1];
    char email[EMAIL_LENGTH + 1];
    int32_t next;
    // bool del;
} Row;

const int32_t ID_SIZE       = SIZE_OF_ATTRIBUTE(Row, id);
const int32_t ONLINE_SIZE   = SIZE_OF_ATTRIBUTE(Row, isOnline);
const int32_t USERNAME_SIZE = SIZE_OF_ATTRIBUTE(Row, username);
const int32_t EMAIL_SIZE    = SIZE_OF_ATTRIBUTE(Row, email);
const int32_t NEXT_SIZE     = SIZE_OF_ATTRIBUTE(Row, next);

const int32_t ID_OFFSET         = OFFSET_OF_ATTRIBUTE(Row, id);
const int32_t ONLINE_OFFSET     = OFFSET_OF_ATTRIBUTE(Row, isOnline);
const int32_t USERNAME_OFFSET   = OFFSET_OF_ATTRIBUTE(Row, username);
const int32_t EMAIL_OFFSET      = OFFSET_OF_ATTRIBUTE(Row, email);
const int32_t ROW_SIZE          = ID_SIZE + ONLINE_SIZE + USERNAME_SIZE + EMAIL_SIZE + NEXT_SIZE;
const int32_t MAX_ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;

/**
 * 
 * Page 在磁盘上的结构:
 * +----------+------------------+-----------------+------+------+------+
 * | rowCount | lastModifiedTime | lastModifiedRow | row0 | row1 | rowN | 
 * +----------+------------------+-----------------+------+------+------+
 * rowCount: count of rows in this page
 * lastModifiedTime: last modified time in this page
 * lastRowOffset: byte offset of last row in this page
 */
typedef struct Page {
    /* page headers */
    int32_t rowCount;
    int32_t lastModifiedRow;
    int32_t lastReadRow;
    int32_t firstFree;
    int32_t firstUse;

    time_t lastModifyTime;
    time_t lastReadTime;
    /* rows */
    Row *rows[MAX_ROWS_PER_PAGE];
} Page;

const uint32_t ROWCOUNT_SIZE           = SIZE_OF_ATTRIBUTE(Page, rowCount);
const uint32_t LASTMODIFIEDROW_SIZE    = SIZE_OF_ATTRIBUTE(Page, lastModifiedRow);
const uint32_t LASTREADROW_SIZE        = SIZE_OF_ATTRIBUTE(Page, lastReadRow);
const uint32_t LASTMODIFIEDTIME_SIZE   = SIZE_OF_ATTRIBUTE(Page, lastModifyTime);
const uint32_t LASTREADTIME_SIZE       = SIZE_OF_ATTRIBUTE(Page, lastReadTime);
const uint32_t FIRST_FREE_SIZE         = SIZE_OF_ATTRIBUTE(Page, firstFree);
const uint32_t FIRST_USE_SIZE          = SIZE_OF_ATTRIBUTE(Page, firstUse);
const uint32_t ROWCOUNT_OFFSET         = OFFSET_OF_ATTRIBUTE(Page, rowCount);
const uint32_t LASTMODIFIEDROW_OFFSET  = OFFSET_OF_ATTRIBUTE(Page, lastModifiedRow);
const uint32_t LASTMODIFIEDTIME_OFFSET = OFFSET_OF_ATTRIBUTE(Page, lastModifyTime);
const uint32_t ROWS_OFFSET             = OFFSET_OF_ATTRIBUTE(Page, rows);

const uint32_t PAGE_HEADER_SIZE = ROWCOUNT_SIZE + LASTREADROW_SIZE + LASTMODIFIEDROW_SIZE + LASTREADTIME_SIZE + LASTMODIFIEDTIME_SIZE + FIRST_FREE_SIZE + FIRST_USE_SIZE;

typedef struct Pager {
    int fd; /* file descriptor */
    off_t fileLength;
    int32_t pageCount;
    char file[];
} Pager;

typedef struct Table {
    int32_t rowCount;
    Pager *pager;
} Table;

TINYDB_API Pager *New_Pager(const char *file);
TINYDB_API void Destroy_Pager(Pager *pager);

TINYDB_API PagerExecuteResult Pager_Insert(Pager *pager, Row *row);
TINYDB_API PagerExecuteResult Pager_Select(Pager *pager, KEY id, Row **ret);
TINYDB_API PagerExecuteResult Pager_Update(Pager *pager, KEY id, Row *row, Row **ret);
TINYDB_API PagerExecuteResult Pager_Delete(Pager *pager, KEY id, Row **ret);

/* private functions */
#ifdef DEBUG_TEST
Row *New_Row();
Page *New_Page();
void CreateFileIfNotExists(const char *file);
int OpenFile(const char *file);
void CloseFile(int fd);
// inline Row *PagerSearchCache(Pager *pager, KEY id);
inline int32_t PagerSearchPage(Pager *pager, KEY id);
inline int32_t PageSearchRow(Page *page, KEY id);
inline void SerializePage(Page *page, void *buffer);
inline void DeserializePage(Page *page, void *buffer);
#endif

#endif