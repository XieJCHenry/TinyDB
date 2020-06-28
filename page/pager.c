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
#include <unistd.h>

/**
 * TODO: implement the buffer module.
 * 
 * 
 */
/* ==================== PRIVATE ==================== */
#ifndef DEBUG_TEST
PRIVATE
#endif
Record *New_Row() {
    Record *row = (Record *)calloc(1, sizeof(Record));
    assert(row != NULL);
    row->id       = -1;
    row->isOnline = false;
    return row;
}

#ifndef DEBUG_TEST
PRIVATE
#endif
Page *New_Page() {
    Page *page = (Page *)calloc(1, sizeof(Page));
    assert(page != NULL);
    page->rowCount         = 0;
    page->lastModifiedRow  = -1;
    page->lastModifiedTime = -1;
    page->lastRowOffset    = -1;
    for (uint32_t i = 0; i < MAX_ROWS_PER_PAGE; i++) {
        page->rows[i] = New_Row();
        assert(page->rows[i] != NULL);
    }
    return page;
}

#ifndef DEBUG_TEST
PRIVATE
#endif
void Destroy_Page(Page *page) {
    uint32_t i;
    for (i = 0; i < page->rowCount; i++) {
        if (page->rows[i] != NULL) {
            free(page->rows[i]);
            page->rows[i] = NULL;
        }
    }
}
/**
 * Size of file is always times of 4096 bytes. 
 */
#ifndef DEBUG_TEST
PRIVATE
#endif
void CreateFileIfNotExists(const char *file) {
    if (file == NULL || strlen(file) == 0) {
        EXIT_ERROR("file is not exists.\n");
    }
    if (access(file, F_OK) == 0) {
        printf("%s is already exists.\n", file);
        if (access(file, R_OK) == 0) {
            printf("%s can read,", file);
        }
        if (access(file, W_OK) == 0) {
            printf("write");
        }
        printf(".\n");
    } else {
        printf("%s is not exists, it will be create later.\n", file);
        // 创建一个文件，大小为一个pagesize。
        FILE *fp = fopen(file, "wb+");
        if (fp == NULL) {
            EXIT_ERROR("Cannot create file.\n");
        }
        fseek(fp, PAGE_SIZE - 1, SEEK_SET);
        fputc(1, fp);  // 在PAGE_SIZE-1处放置一个整数
        fclose(fp);
    }
}

#ifndef DEBUG_TEST
PRIVATE
#endif
int OpenFile(const char *file) {
    assert(file != NULL);
    size_t len = strlen(file);
    assert(len > 0 && len < MAX_DB_FILE_LENGTH);

    CreateFileIfNotExists(file);
    int fd = open(file, O_RDWR);  // O_RDWR，以读写模式打开；O_WRONLY，以只写模式打开；O_RDONLY，以只读模式打开
    if (fd == -1) {
        EXIT_ERROR("Must provide an existed file.\n");
    }
    return fd;
}

#ifndef DEBUG_TEST
PRIVATE
#endif
PagerExecuteResult CloseFile(int fd) {
    if (fd == -1) {
        EXIT_ERROR("Must provide an existed file.\n");
    }
    if (close(fd) == -1) {
        EXIT_ERROR("Failed to close file.\n");
    }
    return Pager_ExecuteSuccess;
}

#ifndef DEBUG_TEST
PRIVATE
#endif
inline void SerializePage(Page *page, void **buffer) {
    assert(page != NULL);
    assert(buffer != NULL);
    void *bufptr = *buffer;
    memcpy(bufptr, page, sizeof(char) * PAGEHEADER_SIZE);
    for (int i = 0; i < page->rowCount; i++) {
        memcpy(bufptr + PAGEHEADER_SIZE + i * ROW_SIZE, page->rows[i], ROW_SIZE);
    }
}

#ifndef DEBUG_TEST
PRIVATE
#endif
inline void DeserializePage(Page **page, void *buffer) {
    Page *p = *page;
    /* deserialize page header */
    // memcpy(&(p->rowCount), buffer, ROWCOUNT_SIZE);
    // memcpy(&(p->lastModifiedRow), buffer + LASTMODIFIEDROW_OFFSET, LASTMODIFIEDROW_SIZE);
    // memcpy(&(p->lastRowOffset), buffer + LASTROWOFFSET_OFFSET, LASTROWOFFSET_SIZE);
    // memcpy(&(p->lastModifiedTime), buffer + LASTMODIFIEDTIME_OFFSET, LASTMODIFIEDTIME_SIZE);
    memcpy(p, buffer, PAGEHEADER_SIZE);
    /* deserialize page rows */
    for (int i = 0; i < p->rowCount; i++) {
        memcpy(p->rows[i], buffer + ROWS_OFFSET + i * ROW_SIZE, ROW_SIZE);
    }
}
/* ==================== PAGER_API ==================== */

TINYDB_API inline uint32_t PagerSearchPage(Pager *pager, KEY id) {
    uint32_t i;
    for (i = 0; i < pager->pageCount; i++) {
        Page *page = pager->cache[i];
        if (page->rows[page->rowCount - 1]->id >= id) {
            return i;
        }
    }
    return -1;
}

TINYDB_API inline uint32_t PageSearchRow(Page *page, KEY id) {
    uint32_t i;
    for (i = 0; i < page->rowCount && page->rows[i]->id != id; i++)
        ;
    return i == page->rowCount ? -1 : i;
}

TINYDB_API void PrintPage(Page *page) {
    if (page == NULL) {
        printf("Page is null.\n");
    }
    printf("----------------------------------------\n");
    printf("page->rowCount = %d\n", page->rowCount);
    printf("page->lastModifiedRow = %d\n", page->lastModifiedRow);
    printf("page->lastModifiedTime = %ld\n", page->lastModifiedTime);
    printf("page->lastRowOffset = %d\n", page->lastRowOffset);
    int i;
    for (i = 0; i < page->rowCount; i++) {
        Record *row = page->rows[i];
        printf("===================\n");
        printf("row->id = %d\n", row->id);
        printf("row->isOnline = %d\n", row->isOnline);
        printf("row->username = %s\n", row->username);
        printf("row->email = %s\n", row->email);
        printf("===================\n");
    }
    printf("----------------------------------------\n");
}

/**
 * @param file: storage data
 */
TINYDB_API Pager *New_Pager(const char *file) {
    int fd       = OpenFile(file);
    Pager *pager = (Pager *)calloc(1, sizeof(Pager) + sizeof(char) * (strlen(file) + 1));
    assert(pager != NULL);

    strcpy(pager->file, file);
    pager->fd         = fd;
    pager->fileLength = lseek(fd, 0L, SEEK_END);
    pager->pageCount  = pager->fileLength / PAGE_SIZE;
    lseek(fd, 0L, SEEK_SET);  // reset fd
    pager->cachePageNum = 0;

    return pager;
}

TINYDB_API void Destroy_Pager(Pager *pager) {
    assert(pager != NULL);
    CloseFile(pager->fd);
    uint32_t i;
    for (i = 0; i < pager->cachePageNum; i++) {
        if (pager->cache[i] != NULL) {
            Destroy_Page(pager->cache[i]);
        }
    }
    free(pager);
    pager = NULL;
}

/**
 * Version1: Only append
 * 
 * Version2: All insertion are written to the cache and finally to disk by Pager_Flush() function.
 * TODO: Now it's a problem: how to manage the alignment of a record in the file?
 */
TINYDB_API PagerExecuteResult Pager_Insert(Pager *pager, Record *row) {
    assert(row != NULL);
    // 1. locate the last page
    off_t seekIdx = pager->fileLength - PAGE_SIZE;
    if (lseek(pager->fd, seekIdx, SEEK_SET) == -1) {
        EXIT_ERROR("Error lseek.\n");
    }
    // 2. 读取该page的rowCount
    void *buffer = alloca(sizeof(char) * PAGE_SIZE);
    if (read(pager->fd, buffer, PAGE_SIZE) == -1) {
        EXIT_ERROR("Fail to read 4K bytes at the end of file.\n");
    }
    uint32_t rowCount;
    memcpy(&rowCount, buffer, sizeof(uint32_t));
    if (rowCount < MAX_ROWS_PER_PAGE) {
        // 2.1 if rowCount < MAX_ROWS_PER_PAGE
        // insert directly
        Page *page = New_Page();
        DeserializePage(&page, buffer);
        page->rows[page->rowCount] = row;
        page->lastModifiedRow      = page->rowCount;
        page->lastRowOffset += ROW_SIZE;
        page->lastModifiedTime = time(NULL);
        page->rowCount++;
        SerializePage(page, &buffer);
        PrintPage(page);
        // write to file
        if (lseek(pager->fd, seekIdx, SEEK_SET) == -1) {
            EXIT_ERROR("Error lseek.\n");
        }
        if (write(pager->fd, buffer, PAGE_SIZE) != PAGE_SIZE) {
            EXIT_ERROR("Error occured when write page to file.\n");
        }
        free(page);
        page = NULL;
    } else {
        // 2.2 if rowCount == MAX_ROWS_PER_PAGE
        // create a new page
        Page *page             = New_Page();
        page->rowCount         = 1;
        page->lastModifiedRow  = 0;
        page->lastModifiedTime = time(NULL);
        page->rows[0]          = row;
        SerializePage(page, &buffer);
        if (lseek(pager->fd, 0L, SEEK_END) == -1) {
            EXIT_ERROR("Error lseek.\n");
        }
        if (write(pager->fd, buffer, PAGE_SIZE) != PAGE_SIZE) {
            EXIT_ERROR("Error occured when write page to file.\n");
        }
        free(page);
        page = NULL;
    }
    return Pager_ExecuteSuccess;
}

/**
 * Version 1:
 * Select a row directly without passing through the buffer.
 * Read the entire page form disk, then storage it to the buffer.
 * 
 * Version 2:
 * The buffer is first looked up, and if there is a hit, the result returns, 
 * or there is no hit, then perform disk io.
 * 
 */
TINYDB_API PagerExecuteResult Pager_Select(Pager *pager, Cursor *cursor, KEY id, Record **ret) {
    uint32_t i, j;
    // 1. search buffer
    for (i = 0; i < pager->cachePageNum; i++) {
        Page *page = pager->cache[i];
        for (j = 0; j < page->rowCount; j++) {
            // if hit, return
            if (page->rows[j]->id == id) {
                *ret = page->rows[i];
                return Pager_ExecuteSuccess;
            }
        }
    }
    printf("Pager_Select: Record is not in buffer.\n");
    // 2.1 if not, search disk, then append the new page to the buffer, here needs a replacement strategy
    uint32_t pageOffset, byteOffset;
    pageOffset = cursor->index / PAGE_SIZE;
    if (lseek(pager->fd, pageOffset, SEEK_SET) == -1) {
        EXIT_ERROR("Error lseek.\n");
    }
    void *buffer = alloca(sizeof(char) * PAGE_SIZE);
    if (read(pager->fd, buffer, PAGE_SIZE) == -1) {
        perror("Error read.\n");
        *ret = NULL;
        return Pager_ExecuteFailed;
    }
    Page *page = New_Page();
    DeserializePage(&page, buffer);
    for (i = 0; i < page->rowCount; i++) {
        if (page->rows[i]->id == id) {
            // Copying a backup to ret makes both of them independent.
            memcpy(*ret, page->rows[i], ROW_SIZE);
        }
    }
    // 2.2 append the new page to buffer or replace an obsolete page by strategy.
    if (pager->cachePageNum < BUFFER_MAX_PAGES) {
        pager->cache[pager->cachePageNum++] = page;
    } else {
        // TODO: implement the clock algorithm.
        perror("Buffer replacement strategy is not implemented yet, so just replce the oldest page.\n");
        Page *old       = pager->cache[0];
        pager->cache[0] = page;
        Destroy_Page(pager->cache[0]);
        old = NULL;
    }
    return Pager_ExecuteSuccess;
}

/**
 * Update a row directly without passing through the buffer.
 * @param ret: return the old row
 */
TINYDB_API PagerExecuteResult Pager_Update(Pager *pager, Cursor *cursor, Record *row, Record **ret) {
    uint32_t byteOffset = ROW_BYTE_OFFSET(cursor->index, PAGE_SIZE, ROW_SIZE, ROWS_OFFSET);
    if (lseek(pager->fd, byteOffset, SEEK_SET) == -1) {
        EXIT_ERROR("Error lseek.\n");
    }
    // read the old row
    if (read(pager->fd, *ret, ROW_SIZE) == -1) {
        perror("Error read.\n");
        *ret = NULL;
        return Pager_ExecuteFailed;
    }
    // seek back and write
    lseek(pager->fd, -ROW_SIZE, SEEK_SET);
    if (write(pager->fd, row, ROW_SIZE) != ROW_SIZE) {
        perror("Error write but read success.\n");
        return Pager_ExecuteFailed;
    }
    return Pager_ExecuteSuccess;
}

/**
 * Markable delete passing through the buffer.
 * TODO: need to implement a buffer.
 */
TINYDB_API PagerExecuteResult Pager_Delete(Pager *pager, Cursor *cursor, DBMetaData *metaData, Record **ret) {
    *ret = NULL;
    perror("Pager_Delete is always return Pager_ExecuteFailed.\n");
    return Pager_ExecuteFailed;
}

/**
 * flush pages storage in buffer to disk
 * 
 */
TINYDB_API PagerExecuteResult Pager_Flush(Pager *pager) {
    perror("Pager_Flush is not implemented yet.\n");
    return Pager_ExecuteFailed;
}