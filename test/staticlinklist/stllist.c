#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define SIZE 10

typedef struct node_t {
    int val;
    int next;
} Node;

typedef struct staticLinkList_t {
    int firstUse;
    int firstFree;
    int useNum;
    Node *nodes[SIZE];
} StaticLinkList, StlList;

/**
 * firstFree 记录空闲链表第一个可用结点，
 * 摘取结点时，判断是否还有剩余空间。
 * 如果没有剩余结点，firstFree的值是-1；
 * 如果有剩余结点，firstFree的值就是第一个空闲结点的下标，
 * 摘取后，将firstFree的值设置为第一个空闲结点的后继结点的下标。
 * 
 */
static inline int mallocNode(StlList *list) {
    int i = list->firstFree;
    if (list->firstFree != -1) {
        list->firstFree = list->nodes[i]->next;
    }
    return i;
}

/**
 * 将第k个结点回收到freeList中
 */
static inline void freeNode(StlList *list, int k) {
    list->nodes[k]->next = list->firstFree;
    list->firstFree      = k;
}

StlList *New_StlList() {
    StlList *list = (StlList *)calloc(1, sizeof(StlList));
    if (list == NULL) {
        return NULL;
    }
    list->firstUse  = -1;
    list->firstFree = -1;
    int i;
    for (i = 0; i < SIZE; i++) {
        list->nodes[i] = (Node *)calloc(1, sizeof(Node));
    }
    return list;
}

/**
 * 初始化时，将所有结点都加入到freeList中 
 * 
 */
void Init_StlList(StlList *list) {
    // 先将所有结点都按顺序链接到freeList中
    list->firstFree = 0;
    int i;
    for (i = 0; i < SIZE; i++) {
        list->nodes[i]->next = i + 1;
    }
    list->nodes[i - 1]->next = -1;
}

/**
 * 就将val插入到合适位置，使得整个链表遍历结果为升序
 */
void StlList_Insert(StlList *list, int val) {
    int prev, ptr;
    ptr = list->firstUse;
    while (ptr != -1 && list->nodes[ptr]->val < val) {
        prev = ptr;
        ptr  = list->nodes[ptr]->next;
    }
    int i = mallocNode(list);
    if (i == -1) {
        printf("list is full.\n");
        return;
    }
    list->nodes[i]->val = val;
    if (ptr == list->firstUse) {  // 插入在头部
        list->nodes[i]->next = list->firstUse;
        list->firstUse       = i;
    } else if (ptr == -1) {  // 插入位置在末尾
        list->nodes[prev]->next = i;
        list->nodes[i]->next    = -1;
    } else {
        list->nodes[prev]->next = i;
        list->nodes[i]->next    = ptr;
    }
    list->useNum++;
}

void StlList_Delete(StlList *list, int val) {
    if (list->firstUse == -1) {
        printf("List is empty.\n");
        return;
    }
    // 找到删除位置
    int prev, ptr;
    ptr = list->firstUse;
    while (ptr != -1 && list->nodes[ptr]->val != val) {
        prev = ptr;
        ptr  = list->nodes[ptr]->next;
    }
    if (ptr == -1) {
        printf("val = %d is not in list.\n", val);
        return;
    } else if (ptr == list->firstUse) {
        list->firstUse = list->nodes[ptr]->next;
    } else {
        list->nodes[prev]->next = list->nodes[ptr]->next;
    }
    freeNode(list, ptr);
    list->useNum--;
}

void StlList_PrintAll(StlList *list) {
    if (list == NULL)
        return;
    printf("\n");
    int ptr = list->firstUse;
    while (ptr != -1) {
        printf("val = %d, ", list->nodes[ptr]->val);
        ptr = list->nodes[ptr]->next;
    }
    printf("\n");
}

int main(int argc, char const *argv[]) {
    StlList *list = New_StlList();
    Init_StlList(list);

    int arr[SIZE + 1] = {11, 12, 15, 16, 14, 13, 10, 19, 8, 17, 2};
    int i;
    for (i = 0; i < SIZE + 1; i++) {
        StlList_Insert(list, arr[i]);
    }
    StlList_PrintAll(list);

    StlList_Delete(list, 23);
    StlList_PrintAll(list);
    StlList_Delete(list, 8);
    StlList_PrintAll(list);
    StlList_Delete(list, 16);
    StlList_PrintAll(list);
    StlList_Delete(list, 19);
    StlList_PrintAll(list);

    return 0;
}