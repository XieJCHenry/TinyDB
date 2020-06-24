/**
 * 在B+树索引下，数据文件中的记录都是按照rowId由大到小有序排列的。
 * 
 * 
 * 
 * 
 */

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
    for (int32_t i = 0; i < MAX_ROWS_PER_PAGE; i++) {
        page->rows[i] = (Row *)calloc(1, sizeof(Row));
        assert(page->rows[i] != NULL);
    }
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
            printf("write", file);
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
FILE *OpenFile(const char *file) {
    assert(file != NULL);
    size_t len = strlen(file);
    assert(len > 0 && len < MAX_DB_FILE_LENGTH);

    CreateFileIfNotExists(file);
    FILE *fp = fopen(file, "rb+");  // 以打开文件进行读写，要求文件必须存在
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
inline int32_t PagerSearchPage(Pager *pager, KEY id) {
    int32_t i;
    for (i = 0; i < pager->pageCount; i++) {
        Page *page = pager->pages[i];
        if (page->rows[page->rowCount - 1]->id >= id) {
            return i;
        }
    }
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

/**
 * 
 */
#ifndef DEBUG_TEST
PRIVATE
#endif
inline void *SerializePage(Page *page) {
    void *ret = calloc(1, sizeof(char) * PAGE_SIZE);
    // memcpy()
    // int32_t rowCountOffset = 0;
    // int32_t lastModifiedRowOffset = rowC
}

/**
 * 从指定位置读取一个Page大小的数据
 */
#ifndef DEBUG_TEST
PRIVATE
#endif
inline Page *DeserializePage(void *src) {
    int32_t lastRowOffset, rowCount, lastModifiedRow;
    clock_t lastModifiedTime;
    memcpy(&(rowCount), src, sizeof(int32_t));
    memcpy(&(lastModifiedTime), src + sizeof(int32_t), sizeof(clock_t));
    memcpy(&(lastModifiedRow), src + sizeof(int32_t) + sizeof(clock_t), sizeof(int32_t));
    memcpy(&lastRowOffset, src + sizeof(int32_t) * 2 + sizeof(clock_t), sizeof(int32_t));

    // Page *newPage = (Page *)calloc(1, sizeof(Page) + sizeof(Row) * rowCount);
    // assert(newPage != NULL);
    Page *newPage             = New_Page();
    newPage->rowCount         = rowCount;
    newPage->lastModifiedRow  = lastModifiedRow;
    newPage->lastModifiedTime = lastModifiedTime;
    newPage->lastRowOffset    = lastRowOffset;
    int32_t pageHeaderSize    = sizeof(int32_t) * 3 + sizeof(clock_t);
    for (int32_t i = 0; i < rowCount; i++) {
        memcpy(newPage->rows[i], src + pageHeaderSize + (i * ROW_SIZE), ROW_SIZE);
    }
    return newPage;
}

/*************************************************************/

DBMetaData *New_MetaData(char *file) {
    if (file == NULL || strlen(file) == 0) {
        EXIT_ERROR("Path of DBMetaData is empty.\n");
    }

    FILE *fp = fopen(file, "rb");
    if (fp == NULL) {
        EXIT_ERROR("DBMetaData is not exists.\n");
    }

    DBMetaData *metaData = (DBMetaData *)calloc(1, sizeof(DBMetaData));
    assert(metaData != NULL);

    size_t read = fread(&(metaData->firstDeletedOffset), sizeof(metaData->firstDeletedOffset), 1, fp);
    if (read == 0) {
        EXIT_ERROR("Failed to read metaData.\n");
    }

    if (fclose(fp) != 0) {
        EXIT_ERROR("Failed to close metaData.\n");
    }

    return metaData;
}

/**
 * @param file: storage data
 */
TINYDB_API Pager *New_Pager(const char *file) {
    FILE *fp     = OpenFile(file);
    Pager *pager = (Pager *)calloc(1, sizeof(Pager) + sizeof(char) * (strlen(file) + 1));
    assert(pager != NULL);

    strcpy(pager->file, file);
    pager->fp = fp;
    fseek(fp, 0L, SEEK_END);        // fseek doesn't return its position
    pager->fileLength = ftell(fp);  // ftell return the position of file pointer
    pager->pageCount  = 0;

    return pager;
}

TINYDB_API void Destroy_Pager(Pager *pager) {
    assert(pager != NULL);

    CloseFile(pager->fp);
    // free(pager->pages);
    for (int32_t i = 0; i < pager->pageCount; i++) {
        free(pager->pages[i]);
    }
    free(pager->file);
    free(pager);
    pager = NULL;
}

/**
 * Pager insert a row
 * 目前只能实现在末尾插入。
 * ---------------------------------------
 * 在任意位置插入的实现思路为：
 * 先在文件中找到插入位置idx，然后开辟一块内存，将idx后面的所有内容都复制到内存里，
 * 然后插入新记录，将内存里的数据都append到末尾即可。
 * ---------------------------------------
 */
#ifndef DEBUG_TEST
#else
/**
 * Only append, and don't close the file.
 * 
 * pager is required to hold a file that is opened.
 * Size of the file is always times of 4096.
 * 
 * TODO:
 */
TINYDB_API PagerExecuteResult Pager_Insert(Pager *pager, Row *row) {
    FILE *fp     = pager->fp;
    long seekIdx = pager->fileLength - PAGE_SIZE;
    int fsret    = fseek(fp, seekIdx, SEEK_SET);
    if (fsret == -1) {
        EXIT_ERROR("Failed to fseek file.\n");
    }
    void *temp  = calloc(1, sizeof(char) * PAGE_SIZE);           // allocate from stack
    size_t read = fread(temp, sizeof(char) * PAGE_SIZE, 1, fp);  // fread return count of element
    if (read != 1) {
        EXIT_ERROR("Fail to read 4096 bytes.\n");
    }

    printf("position of fp = %ld\n", ftell(fp));
    int32_t rowCount = 0;
    memcpy(&rowCount, temp, sizeof(int32_t));
    if (rowCount < MAX_ROWS_PER_PAGE) {
        Page *lastPage = DeserializePage(temp);
        memcpy(lastPage->rows + lastPage->rowCount, row, sizeof(char) * ROW_SIZE);
        lastPage->lastModifiedRow = lastPage->rowCount;
        lastPage->rowCount++;
        lastPage->lastModifiedTime = clock();
        lastPage->lastRowOffset += ROW_SIZE;
        fseek(fp, -PAGE_SIZE, SEEK_CUR);
        // serializePage()
        // fwrite(lastPage, sizeof(char) * PAGE_SIZE, 1, fp);
        free(lastPage->rows);
        free(lastPage);
    } else {
        // create a new Page
        Page *page = New_Page();
        memcpy(page->rows, row, sizeof(char) * ROW_SIZE);
        page->rowCount         = 1;
        page->lastModifiedRow  = 0;
        page->lastModifiedTime = clock();
        fseek(fp, 0L, SEEK_END);
        fwrite(page, sizeof(char) * PAGE_SIZE, 1, fp);
        free(page->rows);
        free(page);
    }
    free(temp);
    fflush(fp);

    return Pager_ExecuteSuccess;
}
#endif

// TODO: Test
TINYDB_API PagerExecuteResult Pager_Select(Pager *pager, Cursor *cursor, KEY id, Row **ret) {
    int32_t pageIdx, rowIdx;
    pageIdx       = PagerSearchPage(pager, id);
    Page *page    = pager->pages[pageIdx];
    rowIdx        = PageSearchRow(page, id);
    Row *cacheRet = page->rows[rowIdx];
    if (cacheRet != NULL) {
        *ret = cacheRet;
        return Pager_ExecuteSuccess;
    }

    // 2. 若查找失败，再读取文件。
    int32_t pageOffset = cursor->index / MAX_ROWS_PER_PAGE;
    FILE *fp           = pager->fp;
    int skret          = fseek(fp, pageOffset, SEEK_SET);
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

    for (int32_t i = 0; i < newPage->rowCount; i++) {
        if (newPage->rows[i]->id == id) {
            *ret = newPage->rows[i];
            break;
        }
    }
    pager->pages[pager->pageCount++] = newPage;
    fseek(fp, 0L, SEEK_SET);  // 将文件指针定位回到文件头
    return Pager_ExecuteSuccess;
}

/**
 * 缺点：只修改了cache，但是没有修改文件
 * 如果是修改 id，需要重新排序
 */
TINYDB_API PagerExecuteResult Pager_Update(Pager *pager, Cursor *cursor, KEY id, Row *row, Row **ret) {
    int32_t idx = PagerSearchPage(pager, id);
    Page *page  = pager->pages[idx];
    if (page == NULL) {
        return Pager_ExecuteFailed;
    }
    int32_t i, j;
    i = 0, j = page->rowCount;
    while (i < j) {
        int32_t mid = i + (j - i) / 2;
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
            if (row1->isOnline != row->isOnline) {
                row1->isOnline = row->isOnline;
            }
            page->lastModifiedTime = clock();
            page->lastModifiedRow  = i;
            return Pager_ExecuteSuccess;
        }
    }
}

/**
 * 思路：
 * 根据cursor计算出row在db file中的偏移量，清空其内容，按照链表插入节点的方法，
 * 找到该位置的前一个空闲位和后一个空闲位，更新值。
 */
TINYDB_API PagerExecuteResult Pager_Delete(Pager *pager, Cursor *cursor, DBMetaData *metaData, Row **ret) {
    int32_t pageOffset = cursor->index / MAX_ROWS_PER_PAGE;
    int32_t rowOffset  = cursor->index % MAX_ROWS_PER_PAGE;
    int64_t byteOffset = pageOffset * PAGE_SIZE + rowOffset;  // 计算被删除行在文件中的偏移量

    if (pager->fileLength < byteOffset) {
        perror("byte offset of row to deleted is over than fileLength\n");
        return Pager_ExecuteFailed;
    }

    /* TODO: 这样会导致在执行一次删除操作前执行了大量不必要的io操作。可以将其删除位链表存入内存结构中。*/
    int64_t prev = 0, next = metaData->firstDeletedOffset;
    FILE *fp = pager->fp;
    fseek(fp, prev, SEEK_SET);
    bool isDeleted;
    while (1) {
        if (next == byteOffset) {
            isDeleted = true;
            break;
        } else if (next > byteOffset) {  //firstDeletedOffset 初始化为fileLength;
            isDeleted = false;
            break;
        } else if (next < byteOffset) {
            prev = next;
            fread(&next, sizeof(int64_t), 1, fp);
        }
    }

    if (isDeleted) {
        *ret = NULL;
        return Pager_ExecuteSuccess;
    } else {
        // 先将待删除行读入返回值
        fseek(fp, byteOffset, SEEK_SET);
        fread(*ret, ROW_SIZE, 1, fp);
        // 将下一删除位的偏移量存入当前删除位
        fseek(fp, byteOffset, SEEK_SET);
        void *emptyRow = alloca(sizeof(char) * ROW_SIZE);
        memcpy(emptyRow, &next, sizeof(int64_t));
        fwrite(emptyRow, ROW_SIZE, 1, fp);
        // 将当前删除位的偏移量存入前一删除位
        fseek(fp, prev, SEEK_SET);
        memset(emptyRow, 0, sizeof(char) * ROW_SIZE);
        memcpy(emptyRow, &prev, sizeof(int64_t));
        fwrite(emptyRow, ROW_SIZE, 1, fp);

        if (prev == 0) {
            metaData->firstDeletedOffset = byteOffset;
        }
        return Pager_ExecuteSuccess;
    }

    return Pager_ExecuteFailed;
}
