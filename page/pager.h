#ifndef PAGE_PAGER_H
#define PAGE_PAGER_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "../includes/global.h"

/* use g++ compiler */
#define USE_GPLUSPLUS_COMPILE

#define SIZE_OF_ATTRIBUTE(struct, attr) (sizeof(((struct *)0)->attr))

#define USERNAME_LENGTH 31
#define EMAIL_LENGTH 255
#define MAX_DB_FILE_LENGTH 255

#define TABLE_MAX_PAGES 100
#define PAGE_SIZE 4096

#define KEY uint64_t

typedef enum {
    Pager_ExecuteFailed  = -1,
    Pager_ExecuteSuccess = 0,

    Pager_RowNotFound      = 1,
    Pager_RowAleardyExists = 2
} PagerExecuteResult;

typedef struct Row {
    KEY id;
    bool isOnline;
    char username[USERNAME_LENGTH + 1];
    char email[EMAIL_LENGTH + 1];
} Row;

#ifdef USE_GPLUSPLUS_COMPILE

const uint64_t ID_OFFSET         = 0;
const uint64_t ID_SIZE           = SIZE_OF_ATTRIBUTE(Row, id);
const uint64_t ONLINE_OFFSET     = ID_OFFSET + ID_SIZE;
const uint64_t ONLINE_SIZE       = SIZE_OF_ATTRIBUTE(Row, isOnline);
const uint64_t USERNAME_OFFSET   = ONLINE_OFFSET + ONLINE_SIZE;
const uint64_t USERNAME_SIZE     = SIZE_OF_ATTRIBUTE(Row, username);
const uint64_t EMAIL_OFFSET      = USERNAME_OFFSET + USERNAME_SIZE;
const uint64_t EMAIL_SIZE        = SIZE_OF_ATTRIBUTE(Row, email);
const uint64_t ROW_SIZE          = ID_SIZE + ONLINE_SIZE + USERNAME_SIZE + EMAIL_SIZE;
const uint64_t MAX_ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;

#else

#define ID_OFFSET 0
#define ID_SIZE (SIZE_OF_ATTRIBUTE(Row, id))
#define ONLINE_OFFSET ((ID_SIZE) + (ID_OFFSET))
#define ONLINE_SIZE (SIZE_OF_ATTRIBUTE(Row, isOnline))
#define USERNAME_OFFSET ((ONLINE_SIZE) + (ONLINE_OFFSET))
#define USERNAME_SIZE (SIZE_OF_ATTRIBUTE(Row, username))
#define EMAIL_OFFSET ((USERNAME_SIZE) + (USERNAME_OFFSET))
#define EMAIL_SIZE (SIZE_OF_ATTRIBUTE(Row, email))
#define ROW_SIZE ((ID_SIZE) + (ONLINE_SIZE) + (USERNAME_SIZE) + (EMAIL_SIZE))
#define MAX_ROWS_PER_PAGE ((PAGE_SIZE) / (ROW_SIZE))

#endif

/* 每个Page减去rows剩余235字节空闲空间 */
/**
 * 
 * Page 在磁盘上的结构:
 * +----------+------------------+-----------------+---------------+------+------+------+
 * | rowCount | lastModifiedTime | lastModifiedRow | lastRowOffset | row0 | row1 | rowN | 
 * +----------+------------------+-----------------+---------------+------+------+------+
 * rowCount: count of rows in this page
 * lastModifiedTime: last modified time in this page
 * lastRowOffset: byte offset of last row in this page
 */
typedef struct Page {
    /* page headers */
    uint32_t rowCount;
    uint32_t lastModifiedRow;
    clock_t lastModifiedTime;
    /* rows */
    Row *rows[];
} Page;

typedef struct Pager {
    FILE *fp; /* db file */
    uint64_t fileLength;
    uint32_t pageCount;
    Page *pages[TABLE_MAX_PAGES];  // TODO: 后续改成循环队列
    char file[];                   /* db file path */
} Pager;

typedef struct Table {
    uint32_t rowCount;
    Pager *pager;
} Table;

typedef struct Cursor {
    uint32_t index;
    Table *table;
} Cursor;

TINYDB_API Pager *New_Pager(char *file);
TINYDB_API void Destroy_Pager(Pager *pager);

TINYDB_API PagerExecuteResult Pager_Insert(Pager *pager, Row *row);
TINYDB_API PagerExecuteResult Pager_Select(Pager *pager, Cursor *cursor, KEY id, Row **ret);
TINYDB_API PagerExecuteResult Pager_Update(Pager *pager, Cursor *cursor, KEY id, Row *row, Row **ret);
TINYDB_API PagerExecuteResult Pager_Delete(Pager *pager, Cursor *cursor, KEY id, Row **ret);

/* private functions */
#ifdef DEBUG_TEST
FILE *OpenFile(const char *file);
PagerExecuteResult CloseFile(FILE *fp);
inline Row *PagerSearchCache(Pager *pager, KEY id);
inline Page *PagerSearchPage(Pager *pager, KEY id);
inline Row *PageSearchRow(Page *page, KEY id);
inline void SerializePage();
inline Page *DeserializePage(void *src);
#endif

#endif