#ifndef TEST_TEST_PAGER_H
#define TEST_TEST_PAGER_H

#include <stdbool.h>
#include <stdint.h>

#define SIZE_OF_ATTRIBUTE(struct, attr) (sizeof(((struct *)0)->attr))
#define USERNAME_LENGTH 120
#define EMAIL_LENGTH 225
#define PAGE_SIZE (4 * 1024)

/**
 * 1. 定义一条Row的格式
 * 2. 计算Row的每一个记录项的大小和偏移量
 * 3. 尝试写入到文件中，并读取出来
 * 
 */
typedef struct s_row {
    uint64_t id;
    bool isOnline;
    char username[USERNAME_LENGTH];
    char email[EMAIL_LENGTH];
} Row;

// #define ID_OFFSET 0
// #define ID_SIZE (SIZE_OF_ATTRIBUTE(Row, id))
// #define ONLINE_OFFSET ((ID_SIZE) + (ID_OFFSET))
// #define ONLINE_SIZE (SIZE_OF_ATTRIBUTE(Row, isOnline))
// #define USERNAME_OFFSET ((ONLINE_SIZE) + (ONLINE_OFFSET))
// #define USERNAME_SIZE (SIZE_OF_ATTRIBUTE(Row, username))
// #define EMAIL_OFFSET ((USERNAME_SIZE) + (USERNAME_OFFSET))
// #define EMAIL_SIZE (SIZE_OF_ATTRIBUTE(Row, email))
// #define ROW_SIZE ((ID_SIZE) + (ONLINE_SIZE) + (USERNAME_SIZE) + (EMAIL_SIZE))

// #define MAX_ROWS_PER_PAGE ((PAGE_SIZE) / (ROW_SIZE))

const uint64_t ID_OFFSET       = 0;
const uint64_t ID_SIZE         = SIZE_OF_ATTRIBUTE(Row, id);
const uint64_t ONLINE_OFFSET   = ID_OFFSET + ID_SIZE;
const uint64_t ONLINE_SIZE     = SIZE_OF_ATTRIBUTE(Row, isOnline);
const uint64_t USERNAME_OFFSET = ONLINE_OFFSET + ONLINE_SIZE;
const uint64_t USERNAME_SIZE   = SIZE_OF_ATTRIBUTE(Row, username);
const uint64_t EMAIL_OFFSET    = USERNAME_OFFSET + USERNAME_SIZE;
const uint64_t EMAIL_SIZE      = SIZE_OF_ATTRIBUTE(Row, email);
const uint64_t ROW_SIZE        = ID_SIZE + ONLINE_SIZE + USERNAME_SIZE + EMAIL_SIZE;

const uint64_t MAX_ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;

typedef struct s_page_header {
    
}PageHeader;

/**
 * 一个Page的默认容量为4k，所能容纳的最大Row数量为 4k / sizeof(Row)。
 * 
 */
typedef struct s_page {
    uint32_t rowNum;
    Row *row[];
} Page;

#endif
