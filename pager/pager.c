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

// #include "../includes/global.h"

#ifndef DEBUG_TEST
PRIVATE
#endif
Row *New_Row() {
    Row *row = (Row *)calloc(1, sizeof(Row));
    assert(row != NULL);
    row->id       = -1;
    row->isOnline = false;
    row->next     = -1;
    return row;
}

#ifndef DEBUG_TEST
PRIVATE
#endif
Page *New_Page() {
    Page *page = (Page *)calloc(1, sizeof(Page));
    assert(page != NULL);
    page->rowCount        = 0;
    page->lastModifiedRow = -1;
    page->lastReadRow     = -1;
    page->lastModifyTime  = -1;
    page->lastReadTime    = -1;
    page->firstFree       = -1;
    page->firstUse        = -1;
    int32_t i;
    for (i = 0; i < MAX_ROWS_PER_PAGE; i++) {
        page->rows[i] = New_Row();
    }

    // 初始化静态链表
    page->firstUse = 0;
    for (i = 0; i < MAX_ROWS_PER_PAGE; i++) {
        page->rows[i]->next = i + 1;
    }
    page->rows[i - 1]->next = -1;
    return page;
}

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
    int fd = open(file, O_RDWR);
    if (open(file, O_RDWR) == -1) {
        EXIT_ERROR("Fail to open file.\n");
    }
    return fd;
}

#ifndef DEBUG_TEST
PRIVATE
#endif
void CloseFile(int fd) {
    if (close(fd) == 0) {
        printf("Success to close file.\n");
    } else {
        perror("Failed to close file.\n");
    }
}

#ifndef DEBUG_TEST
PRIVATE
#endif
inline int32_t PagerSearchPage(Pager *pager, KEY id) {
    return -1;
}

#ifndef DEBUG_TEST
PRIVATE
#endif
inline int32_t PageSearchRow(Page *page, KEY id) {
    int32_t i;
    for (i = 0; i < page->rowCount && page->rows[i]->id != id; i++)
        ;
    return i == page->rowCount ? -1 : i;
}

inline void SerializePage(Page *page, void *buffer) {
    memcpy(buffer, page, PAGE_HEADER_SIZE);
    int32_t i;
    for (i = 0; i < page->rowCount; i++) {
        memcpy(buffer + PAGE_HEADER_SIZE + i * ROW_SIZE, page->rows[i], ROW_SIZE);
    }
}

/**
 * 从指定位置读取一个Page大小的数据
 */
#ifndef DEBUG_TEST
PRIVATE
#endif
inline void DeserializePage(Page *page, void *buffer) {
    memcpy(page, buffer, PAGE_HEADER_SIZE);
    int32_t i;
    for (i = 0; i < page->rowCount; i++) {
        memcpy(page->rows[i], buffer + PAGE_HEADER_SIZE + i * ROW_SIZE, ROW_SIZE);
    }
}

/*************************************************************/
// 静态链表
inline int32_t StlList_MallocNode(Page *page) {
    int32_t i = page->firstUse;
    if (page->firstUse != -1) {
        page->firstUse = page->rows[i]->next;
    }
    return i;
}

inline void StlList_FreeNode(Page *page, int32_t k) {
    page->rows[k]->next = page->firstFree;
    page->firstFree     = k;
}

inline bool StlList_Insert(Page *page, Row *row) {
    if (page->firstFree == -1) {
        return false;
    }
    int32_t prev, ptr;
    ptr    = page->firstUse;
    KEY id = row->id;
    while (ptr != -1 && page->rows[ptr]->id < id) {
        prev = ptr;
        ptr  = page->rows[ptr]->next;
    }
    int32_t i   = StlList_MallocNode(page);
    Row *r      = page->rows[i];
    r->id       = row->id;
    r->isOnline = row->isOnline;
    strcpy(r->username, row->username);
    strcpy(r->email, row->email);

    // NOTE: 要先判断“插入在头部”，再判断“末尾”，因为firstUse=-1
    // 如果是一个空表插入新值，先判断末尾将会导致错误.
    if (ptr == page->firstUse) {  // 插入在头部
        page->rows[i]->next = page->firstUse;
        page->firstUse      = i;
    } else if (ptr == -1) {  // 插入在末尾
        page->rows[prev]->next = i;
        page->rows[i]->next    = -1;
    } else {  // 插入在中间
        page->rows[prev]->next = i;
        page->rows[i]->next    = ptr;
    }
    return true;
}

inline bool StlList_Delete(Page *page, KEY id, Row **ret) {
    if (page->firstUse == -1) {
        return false;
    }
    int32_t prev, ptr;
    ptr = page->firstUse;
    while (ptr != -1 && page->rows[ptr]->id != id) {
        prev = ptr;
        ptr  = page->rows[ptr]->next;
    }
    if (ptr == -1) {
        return true;
    } else if (ptr == page->firstUse) {
        page->firstUse = page->rows[ptr]->next;
    } else {
        page->rows[prev]->next = page->rows[ptr]->next;
    }
    *ret = page->rows[ptr];
    StlList_FreeNode(page, ptr);
    return true;
}

inline int32_t StlList_Select(Page *page, KEY id) {
    int32_t ptr = page->firstUse;
    while (ptr != -1 && page->rows[ptr]->id != id) {
        ptr = page->rows[ptr]->next;
    }
    return ptr;
}

/*************************************************************/

/**
 * @param file: storage data
 */
TINYDB_API Pager *New_Pager(const char *file) {
    int fd       = OpenFile(file);
    Pager *pager = (Pager *)calloc(1, sizeof(Pager) + sizeof(char) * (strlen(file) + 1));
    assert(pager != NULL);

    strcpy(pager->file, file);
    pager->fd         = fd;
    off_t ret         = lseek(fd, 0L, SEEK_END);  // fseek doesn't return its position
    pager->fileLength = ret;
    pager->pageCount  = ret / PAGE_SIZE;

    return pager;
}

TINYDB_API void Destroy_Pager(Pager *pager) {
    assert(pager != NULL);

    CloseFile(pager->fd);
    free(pager);
    pager = NULL;
}

/**
 * Only append.
 * 
 * Size of the file is always times of 4096.
 * TODO: 对于溢出的记录，通常结合“借用后继结点的空间”和“设置溢出块”两种方式。现在均不采用。
 * FIXME: TEST
 */
TINYDB_API PagerExecuteResult Pager_Insert(Pager *pager, Row *row) {
    int fd       = pager->fd;
    void *buffer = alloca(sizeof(char) * PAGE_SIZE);
    Page *page   = New_Page();
    // seek to last page.
    lseek(fd, 0L, SEEK_SET);
    ssize_t readRet = read(fd, buffer, PAGE_SIZE);
    printf("readRet = %ld\n", readRet);
    while (readRet == PAGE_SIZE) {
        DeserializePage(page, buffer);
        if (page->rowCount < MAX_ROWS_PER_PAGE) {
            // 判断是否应该插入当前页面：如果id > 当前页面最大记录的id，且也大于下一页某一记录的id，
            // 这样插入后将导致页面记录乱序。因此需要判断。
            uint64_t end = page->rowCount == 0 ? 0 : page->rowCount - 1;
            // if (page->rowCount == 0 || page->rows[end]->id >= row->id) {
            if (page->rows[end]->id == row->id) {
                printf("id = %d is already exists.\n", row->id);
                free(page);
                return Pager_ExecuteFailed;
            }
            // 1. 先将row拷贝到末尾
            Row *r      = page->rows[page->rowCount];
            r->id       = row->id;
            r->isOnline = row->isOnline;
            // r->del      = row->del;
            memcpy(r->username, row->username, sizeof(char) * USERNAME_SIZE);
            memcpy(r->email, row->email, sizeof(char) * EMAIL_SIZE);
            // 2. 再修改next,prev指针
            int64_t ptr = page->firstUse, prev;
            Row *cur;
            while (ptr >= 0 && ptr < MAX_ROWS_PER_PAGE) {
                cur = page->rows[ptr];
                if (cur->id > row->id) {
                    break;
                }
                prev = ptr;
                ptr  = cur->next;
            }
            if (ptr < 0) {  // 说明链表为空，或者达到末尾
                if (page->rowCount == 0) {
                    page->firstUse = 0;
                    r->next        = -1;
                } else {
                    r->next                = page->rows[prev]->next;
                    page->rows[prev]->next = page->rowCount;
                }
            } else {
                // 在中间插入
                r->next                = ptr;
                page->rows[prev]->next = page->rowCount;
            }
            page->rowCount++;
            SerializePage(page, buffer);
            lseek(fd, -PAGE_SIZE, SEEK_CUR);
            write(fd, buffer, PAGE_SIZE);
            printf("Insert row successfully, id = %d\n", row->id);
            free(page);
            return Pager_ExecuteSuccess;
            // }
        }
        readRet = read(fd, buffer, PAGE_SIZE);
    }
    // 执行到这里时，需要在文件末尾开辟新的页，再添加page。
    page->rowCount = 0;
    Row *r         = page->rows[page->rowCount];
    r->id          = row->id;
    r->isOnline    = row->isOnline;
    // r->del         = row->del;
    memcpy(r->username, row->username, sizeof(char) * USERNAME_SIZE);
    memcpy(r->email, row->email, sizeof(char) * EMAIL_SIZE);
    page->lastModifiedRow = 0;
    page->rowCount++;
    page->lastModifyTime = time(NULL);
    SerializePage(page, buffer);
    lseek(fd, 0L, SEEK_END);
    write(fd, buffer, PAGE_SIZE);
    printf("Append new row at the end of file, id = %d\n", row->id);
    free(page);
    return Pager_ExecuteSuccess;
}

TINYDB_API PagerExecuteResult Pager_Insert2(Pager *pager, Row *row) {
    KEY id       = row->id;
    int fd       = pager->fd;
    void *buffer = alloca(sizeof(char) * PAGE_SIZE);
    Page *page   = New_Page();
    lseek(fd, 0L, SEEK_SET);
    ssize_t readRet = read(fd, buffer, PAGE_SIZE);
    printf("readRet = %ld\n", readRet);
    bool flag = false;
    while (readRet == PAGE_SIZE) {
        DeserializePage(page, buffer);
        // 判断是否应该插入到当前页面
        // 如果不是，则后移
        if (page->rowCount < MAX_ROWS_PER_PAGE) {
            if (page->rowCount == 0 || page->rows[page->rowCount - 1]->id > id) {
                StlList_Insert(page, row);
                flag = true;
                break;
            }
        }
        readRet = read(fd, buffer, PAGE_SIZE);
    }
    if (flag) {
        // 将页面写入到文件中
        page->lastModifiedRow = page->rowCount - 1;
        page->lastModifyTime  = time(NULL);
        page->rowCount++;
        SerializePage(page, buffer);
        if (lseek(fd, -PAGE_SIZE, SEEK_CUR) == -1) {
            perror("Failed to seek back : -PAGE_SIZE\n");
        }
        if (write(fd, buffer, PAGE_SIZE) != PAGE_SIZE) {
            perror("Failed to write a page.\n");
        }
        printf("Insert row successfully, id = %d\n", id);
    } else {
        // 如果执行到这里，说明记录应该插入到一个新page
        page->rowCount = 0;
        Row *r         = page->rows[page->rowCount];
        r->id          = row->id;
        r->isOnline    = row->isOnline;
        strcpy(r->username, row->username);
        strcpy(r->email, row->email);
        page->lastModifiedRow = 0;
        page->rowCount++;
        page->lastModifyTime = time(NULL);
        SerializePage(page, buffer);
        lseek(fd, 0L, SEEK_END);
        if (write(fd, buffer, PAGE_SIZE) != PAGE_SIZE) {
            perror("Failed to write a page.\n");
        }
        printf("Append new row at the end of file, id = %d\n", row->id);
    }
    free(page);
    return Pager_ExecuteSuccess;
}

TINYDB_API PagerExecuteResult Pager_Select(Pager *pager, KEY id, Row **ret) {
    int fd       = pager->fd;
    void *buffer = alloca(sizeof(char) * PAGE_SIZE);
    Page *page   = New_Page();
    lseek(fd, 0L, SEEK_SET);
    ssize_t readRet = read(fd, buffer, PAGE_SIZE);
    printf("readRet = %ld\n", readRet);
    while (readRet == PAGE_SIZE) {
        DeserializePage(page, buffer);
        int32_t ptr = page->firstUse, i;
        if (ptr >= 0 && ptr < MAX_ROWS_PER_PAGE) {
            i = StlList_Select(page, id);
            if (i != -1) {
                if (*ret == NULL) {
                    *ret = New_Row();
                }
                memcpy(*ret, page->rows[i], ROW_SIZE);
                page->lastReadTime = time(NULL);
                page->lastReadRow  = i;
                printf("Success to select row = %d\n", id);
                free(page);
                return Pager_ExecuteSuccess;
            }
        }
        readRet = read(fd, buffer, sizeof(char) * PAGE_SIZE);
    }
    free(page);
    return Pager_ExecuteFailed;
}

/**
 */
TINYDB_API PagerExecuteResult Pager_Update(Pager *pager, KEY id, Row *row, Row **ret) {
    int fd       = pager->fd;
    void *buffer = alloca(sizeof(char) * PAGE_SIZE);
    Page *page   = New_Page();
    lseek(fd, 0L, SEEK_SET);
    ssize_t readRet = read(fd, buffer, PAGE_SIZE);
    bool flag       = false;
    int32_t i;
    while (readRet == PAGE_SIZE) {
        DeserializePage(page, buffer);
        i = StlList_Select(page, id);
        if (i != -1) {
            if (*ret == NULL) {
                *ret = New_Row();
            }
            memcpy(*ret, page->rows[i], ROW_SIZE);
            Row *r      = page->rows[i];
            r->isOnline = row->isOnline;
            if (!strcmp(r->email, row->email)) {
                strcpy(r->email, row->email);
            }
            if (!strcmp(r->username, row->username)) {
                strcpy(r->username, row->username);
            }
            flag = true;
            break;
        }
        readRet = read(fd, buffer, PAGE_SIZE);
    }
    if (i == -1) {
        printf("row is not exists, id = %d\n", id);
        free(page);
        return Pager_ExecuteFailed;
    }
    if (flag) {
        page->lastModifiedRow = i;
        page->lastModifyTime  = time(NULL);
        SerializePage(page, buffer);
        lseek(fd, -PAGE_SIZE, SEEK_CUR);
        if (write(fd, buffer, PAGE_SIZE) != PAGE_SIZE) {
            perror("Failed to write a page.\n");
        }
        printf("Success to update row = %d\n", id);
        free(page);
        return Pager_ExecuteSuccess;
    } else {
        printf("row is not exists, id = %d\n", id);
        return Pager_ExecuteFailed;
    }
}

TINYDB_API PagerExecuteResult Pager_Delete(Pager *pager, KEY id, Row **ret) {
    int fd       = pager->fd;
    void *buffer = alloca(sizeof(char) * PAGE_SIZE);
    Page *page   = New_Page();
    lseek(fd, 0L, SEEK_SET);
    ssize_t readRet = read(fd, buffer, PAGE_SIZE);
    while (readRet == PAGE_SIZE) {
        DeserializePage(page, buffer);
        Row *row = NULL;
        if (StlList_Delete(page, id, &row)) {
            if (*ret == NULL) {
                *ret = New_Row();
            }
            if (row != NULL) {
                memcpy(ret, row, ROW_SIZE);
            }
            page->rowCount--;
            page->lastModifyTime = time(NULL);
            SerializePage(page, buffer);
            lseek(fd, -PAGE_SIZE, SEEK_CUR);
            if (write(fd, buffer, PAGE_SIZE) != PAGE_SIZE) {
                perror("Failed to write a page.\n");
            }
            free(page);
            return Pager_ExecuteSuccess;
        }
        readRet = read(fd, buffer, PAGE_SIZE);
    }
    free(page);
    return Pager_ExecuteFailed;
}
