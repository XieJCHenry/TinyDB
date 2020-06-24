#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./pager.h"

void test_open_close_file();
void test_PagerInsert();

int main(int argc, char const *argv[]) {
    // test_PagerInsert();
    const char *file     = "db";
    const char *username = "xiejiachuang";
    const char *email    = "111111@gmail.com";
    Pager *pager         = New_Pager(file);
    //
    int32_t i;
    for (i = 0; i < 70; i++) {
        Row *row      = New_Row();
        row->id       = i + 10;
        row->isOnline = i % 2 == 0;
        memcpy(row->username, username, sizeof(char) * strlen(username));
        memcpy(row->email, email, sizeof(char) * strlen(email));
        PagerExecuteResult res = Pager_Insert(pager, row);
        printf("execute result = %d", res);
        // free(row);
    }
    Destroy_Pager(pager);
    return 0;
}

void test_open_close_file() {
    const char *file = "db";
    FILE *fp         = OpenFile(file);
    if (fp != NULL) {
        printf("Success\n");
    } else {
        printf("Failed.\n");
    }
    CloseFile(fp);
}

void test_PagerInsert() {
    char *file   = "db";
    Pager *pager = New_Pager(file);
    printf("Length of file = %ld\n", pager->fileLength);
    Row *row             = New_Row();
    row->id              = 12133;
    const char *email    = "12345698@qq.com";
    const char *username = "jchenry";
    strcpy(row->email, email);
    strcpy(row->username, username);
    Pager_Insert(pager, row);
}