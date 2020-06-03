#include "./bplustree.h"

void TestInsert();

int main(int argc, char* argv[]) {
    BPlusTree_Init();
    TestInsert();
    BPlusTree_PrintTree();
    BPlusTree_Destroy();
    return 0;
}

void TestInsert() {
    BPlusTree_Insert(1, 1);
    // BPlusTree_PrintTree();
    BPlusTree_Insert(2, 2);
    // BPlusTree_PrintTree();
    BPlusTree_Insert(3, 3);
    // BPlusTree_PrintTree();
    BPlusTree_Insert(4, 4);
    // BPlusTree_PrintTree();
    BPlusTree_Insert(5, 5);
    // BPlusTree_PrintTree();
    BPlusTree_Insert(6, 6);
    // BPlusTree_PrintTree();
    BPlusTree_Insert(7, 7);
    // BPlusTree_PrintTree();
    BPlusTree_Insert(8, 8);
    // BPlusTree_PrintTree();
    BPlusTree_Insert(9, 9);
    // BPlusTree_PrintTree();
    BPlusTree_Insert(10, 10);
    BPlusTree_PrintTree();
   // ERROR: i = 9
    // for (int i = 1; i <= 10; i++) {
    //     BPlusTree_Insert(i, i);
    //     BPlusTree_PrintTree();
    // }
}
