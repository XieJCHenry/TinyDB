#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./pager.h"

const char *file = "dbfile";

void test_open_close_file();
void test_PagerInsert();
void test_Open_And_Write();
void test_Open_And_Write2();
void test_Open_And_Read();
void test_Open_And_Update();
/**
 * 
 * 
 */

int main(int argc, char const *argv[]) {
    // CreateFileIfNotExists(file);
    // test_Open_And_Write();
    test_Open_And_Read();
    // test_Open_And_Update();
    return 0;
}

/**
 * 
 * Page 在磁盘上的结构:
 * +----------+------------------+-----------------+---------------+------+------+------+
 * | rowCount | lastModifiedTime | lastModifiedRow | lastRowOffset | row0 | row1 | rowN | 
 * +----------+------------------+-----------------+---------------+------+------+------+
 * rowCount: count of rows in this page
 * lastModifiedTime: last modified time in this page
 * lastRowOffset: byte offset of last row in this page
 * 
 * 现在的任务：将内存数据写入到文件中，然后读取出来。
 */

void test_Open_And_Write() {
    const char *username = "xiejiachuang";
    const char *email    = "222211@gmail.com";
    Pager *pager         = New_Pager(file);
    int i;
    for (i = 0; i < 10; i++) {
        Row *row      = New_Row();
        row->id       = i + 27;
        row->isOnline = i % 3 == 0;
        strcpy(row->username, username);
        strcpy(row->email, email);
        Pager_Insert(pager, row);
        free(row);
    }
    Destroy_Pager(pager);
}

void test_Open_And_Read() {
    Pager *pager = New_Pager(file);
    int i;
    for (i = 0; i < 10; i++) {
        Row *row = New_Row();
        row->id  = i + 27;
        Pager_Select(pager, row->id, &row);
        if (row != NULL) {
            printf("row->id=%d\n", row->id);
            printf("row->isOnline = %d\n", row->isOnline);
            printf("row->username = %s\n", row->username);
            printf("row->email = %s\n", row->email);
        } else {
            printf("failed to select row =%d\n", row->id);
        }
        free(row);
    }

    Destroy_Pager(pager);
}

void test_Open_And_Update() {
    Pager *pager = New_Pager(file);
    Row *row     = New_Row();
    Row *nRow    = New_Row();
    Pager_Select(pager, 27, &row);
    row->isOnline = 0;
    Pager_Update(pager, 27, row, &nRow);
    printf("row->id=%d\n", row->id);
    printf("row->isOnline = %d\n", row->isOnline);
    printf("row->username = %s\n", row->username);
    printf("row->email = %s\n", row->email);
    Destroy_Pager(pager);
}
