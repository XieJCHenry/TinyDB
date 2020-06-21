#include "pager.h"

#include <assert.h>
// #include <errno.h>
#include <alloca.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

// #include "../includes/global.h"

#ifndef DEBUG_TEST
PRIVATE
#endif
Row *New_Row() {
    Row *row = (Row *)calloc(1, sizeof(Row));
    assert(row != NULL);
    row->id       = -1;
    row->isOnline = false;
    return row;
}

#ifndef DEBUG_TEST
PRIVATE
#endif
FILE *OpenFile(const char *file) {
    assert(file != NULL);
    size_t len = strlen(file);
    assert(len > 0 && len < MAX_DB_FILE_LENGTH);

    // if (creat(file, O_CREAT | O_RDWR) == -1) {
    //     perror("Error occured when create File.\n");
    //     exit(EXIT_FAILURE);
    // }
    FILE *fp = fopen(file, "rb+");
    if (fp == NULL) {
        printf("Must provide an existed file: %s.\n", file);
        exit(EXIT_FAILURE);
    }
    return fp;
}

#ifndef DEBUG_TEST
PRIVATE
#endif
PagerExecuteResult CloseFile(FILE *fp) {
    if (fp == NULL) {
        return Pager_ExecuteSuccess;
    }
    int ret = fclose(fp);
    if (ret == 0) {
        printf("Success to close file.\n");
        return Pager_ExecuteSuccess;
    } else {
        perror("Failed to close file.\n");
        return Pager_ExecuteFailed;
    }
}

#ifndef DEBUG_TEST
PRIVATE
#endif
inline Row *PagerSearchCache(Pager *pager, KEY id) {
    uint32_t i, j;
    for (i = 0; i < pager->pageCount; i++) {
        Page *page = pager->pages[i];
        if (page->rows[page->rowCount - 1]->id >= id) {
            for (j = page->rowCount - 1; j >= 0; j--) {
                Row *row = page->rows[j];
                if (row->id == id) {
                    return row;
                }
            }
        }
    }
    return NULL;
}

#ifndef DEBUG_TEST
PRIVATE
#endif
inline Page *PagerSearchPage(Pager *pager, KEY id) {
    uint32_t i;
    for (i = 0; i < pager->pageCount; i++) {
        Page *page = pager->pages[i];
        if (page->rows[page->rowCount - 1]->id >= id) {
            return page;
        }
    }
    return NULL;
}

#ifndef DEBUG_TEST
PRIVATE
#endif
inline Row *PageSearchRow(Page *page, KEY id) {
    uint32_t i;
    for (i = 0; i < page->rowCount && page->rows[i]->id != id; i++)
        ;
    return i == page->rowCount ? NULL : page->rows[i];
}

#ifndef DEBUG_TEST
PRIVATE
#endif
inline void SerializePage();

#ifndef DEBUG_TEST
PRIVATE
#endif
inline Page *DeserializePage(void *src) {
    uint32_t lastRowOffset, rowCount, lastModifiedRow;
    clock_t lastModifiedTime;
    memcpy(&(rowCount), src, sizeof(uint32_t));
    memcpy(&(lastModifiedTime), src + sizeof(uint32_t), sizeof(clock_t));
    memcpy(&(lastModifiedRow), src + sizeof(uint32_t) + sizeof(clock_t), sizeof(uint32_t));
    memcpy(&lastRowOffset, src + sizeof(uint32_t) * 2 + sizeof(clock_t), sizeof(uint32_t));

    Page *newPage = (Page *)calloc(1, sizeof(Page) + sizeof(Row) * rowCount);
    assert(newPage != NULL);
    newPage->rowCount         = rowCount;
    newPage->lastModifiedRow  = lastModifiedRow;
    newPage->lastModifiedTime = lastModifiedTime;
    uint32_t pageHeaderSize   = sizeof(uint32_t) * 3 + sizeof(clock_t);
    for (uint32_t i = 0; i < rowCount; i++) {
        memcpy(newPage->rows[i], src + pageHeaderSize + (i * ROW_SIZE), ROW_SIZE);
    }
    return newPage;
}

TINYDB_API Pager *New_Pager(char *file) {
    FILE *fp     = OpenFile(file);
    Pager *pager = (Pager *)calloc(1, sizeof(Pager) + sizeof(char) * (strlen(file) + 1));
    assert(pager != NULL);

    strcpy(pager->file, file);
    pager->fp         = fp;
    pager->fileLength = fseek(fp, 0L, SEEK_END);
    pager->pageCount  = 0;

    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        pager->pages[i] = NULL;
    }
    return pager;
}

TINYDB_API void Destroy_Pager(Pager *pager) {
    assert(pager != NULL);

    CloseFile(pager->fp);
    // free(pager->pages);
    for (uint32_t i = 0; i < pager->pageCount; i++) {
        free(pager->pages[i]);
    }
    free(pager->file);
    free(pager);
    pager = NULL;
}

TINYDB_API PagerExecuteResult Pager_Insert(Pager *pager, Row *row) {
    return Pager_ExecuteFailed;
}

// TODO: Test
TINYDB_API PagerExecuteResult Pager_Select(Pager *pager, Cursor *cursor, KEY id, Row **ret) {
    // 1. 首先在pager->pages中查找，若查找成功，直接返回
    // Row *cacheRet = PagerSearchCache(pager, id);
    Page *page    = PagerSearchPage(pager, id);
    Row *cacheRet = PageSearchRow(page, id);
    if (cacheRet != NULL) {
        *ret = cacheRet;
        return Pager_ExecuteSuccess;
    }

    // 2. 若查找失败，再读取文件。
    uint32_t pageOffset = cursor->index / MAX_ROWS_PER_PAGE;
    FILE *fp            = pager->fp;
    int skret           = fseek(fp, pageOffset, SEEK_SET);
    if (skret != 0) {
        perror("Error when seek the cursor of db file.\n");
        exit(EXIT_FAILURE);
    }
    // 2.2 将对应页面读入内存中
    void *temp  = alloca(sizeof(char) * PAGE_SIZE);  // allocate stack memory, no need to free
    size_t read = fread(temp, sizeof(char) * PAGE_SIZE, 1, fp);
    if (read == 0) {
        perror("Error when read db file.\n");
        exit(EXIT_FAILURE);
    }
    Page *newPage = DeserializePage(temp);

    for (uint32_t i = 0; i < newPage->rowCount; i++) {
        if (newPage->rows[i]->id == id) {
            *ret = newPage->rows[i];
            break;
        }
    }
    pager->pages[pager->pageCount++] = newPage;

    return Pager_ExecuteSuccess;
}

/**
 * 如果是修改 id，需要重新排序
 */
TINYDB_API PagerExecuteResult Pager_Update(Pager *pager, Cursor *cursor, KEY id, Row *row, Row **ret) {
    // Row *cacheRet = PagerSearchCache(pager, row->id);
    Page *page = PagerSearchPage(pager, id);
    if (page == NULL) {
        return Pager_ExecuteFailed;
    }
    uint32_t i, j;
    i = 0, j = page->rowCount;
    while (i < j) {
        uint32_t mid = i + (j - i) / 2;
        if (page->rows[mid]->id < id) {
            i = mid + 1;
        } else if (page->rows[mid]->id > id) {
            j = mid - 1;
        } else {
            *ret      = page->rows[i];
            Row *row1 = page->rows[i];
            if (!strcmp(row1->email, row->email)) {
                strcpy(row1->email, row->email);
            }
            if (!strcmp(row1->username, row->username)) {
                strcpy(row1->username, row->username);
            }
            if (!strcmp(row1->isOnline, row->isOnline)) {
                row1->isOnline = row->isOnline;
            }
            page->lastModifiedTime = clock();
            page->lastModifiedRow  = i;
            return Pager_ExecuteSuccess;
        }
    }
}

TINYDB_API PagerExecuteResult Pager_Delete(Pager *pager, Cursor *cursor, KEY id, Row **ret) {
    return Pager_ExecuteFailed;
}
