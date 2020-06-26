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
/**
 * 
 * 
 */

int main(int argc, char const *argv[]) {
    test_Open_And_Read();
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
    Pager *pager        = New_Pager(file);
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
    Pager_Select(pager, NULL, -1, NULL);  // select all and print
    Destroy_Pager(pager);
}