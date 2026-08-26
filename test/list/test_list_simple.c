#include "os_def.h"
#include "os_dbg.h"
#include "os_lib.h"
#include "os_list.h"

#define TEST_CNT        4

static struct os_test_t {
    const char * name;
    os_list_item_t item;
} test_table[TEST_CNT] = {
    [0] = {.name = "test0"},
    [1] = {.name = "test1"},
    [2] = {.name = "test2"},
    [3] = {.name = "test3"},
};

static os_list_t test_list;     // 测试队列

void os_list_test_simple (void) {
    // 遍历所有结点
    for (int i = 0; i < TEST_CNT; i++) {
        os_list_item_init(&test_table[i].item, struct os_test_t, item);
    }

    os_list_init(&test_list, struct os_test_t, item);

    // 头部添加和移除
    os_printf("\nadd head:\n");
    for (int i = 0; i < TEST_CNT; i++) {
        struct os_test_t * test = test_table + i;
        os_list_insert_first(&test_list, test);
        os_printf("%s\n", test->name);
    }

    os_list_check(&test_list);

    for (int i = 0; i < TEST_CNT; i++) {
        struct os_test_t * test = (struct os_test_t *)os_list_remove_first(&test_list);
        os_printf("%s\n", test->name);
        os_assert(test == test_table + TEST_CNT - 1 - i);
    }
    os_list_check(&test_list);

    // 尾部添加和移除
    os_printf("\nadd tail:\n");
    for (int i = 0; i < TEST_CNT; i++) {
        struct os_test_t * test = test_table + i;
        os_list_insert_last(&test_list, test);
        os_printf("%s\n", test->name);
    }
    os_list_check(&test_list);

    for (int i = 0; i < TEST_CNT; i++) {
        struct os_test_t * test = (struct os_test_t *)os_list_remove_first(&test_list);
        os_printf("%s\n", test->name);
        os_assert(test == test_table + i);
    }
    os_list_check(&test_list);
}
