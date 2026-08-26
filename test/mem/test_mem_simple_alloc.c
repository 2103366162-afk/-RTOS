
#include "os_sys.h"

static cpu_stack_t task1_stack[200];
static cpu_stack_t task2_stack[200];
static cpu_stack_t task3_stack[200];
static cpu_stack_t task4_stack[200];
static os_task_t task1;
static os_task_t task2;
static os_task_t task3;
static os_task_t task4;

// 简单的分配与释放测试
#define ALLOC_COUNT     10
void task1_entry (void * arg) {
	static void * p[ALLOC_COUNT];
	//os_mem_check();

	// 分配内存块并记录下来
	for (int i = 0; i < ALLOC_COUNT; i++) {
		p[i] = os_mem_malloc(i + 1);
		os_printf("alloc mem: 0x%x, size: %d\r\n",(int) p[i], i+1);
		//os_mem_check();
	}

	//释放所有已经分配的内存块
	for (int i = 0; i < ALLOC_COUNT; i++) {
        os_printf("free mem: 0x%x\r\n", (int)p[i]);
		os_men_free(p[i]);
		//os_mem_check();
	}

	for (;;) {
        os_task_delay(500);
	}
}

void task2_entry (void * arg) {
    //int cnt = 0;

	for (;;) {
		//os_printf("task2 is running, %d\n", ++cnt);
        os_task_delay(1000);
	}
}

void task3_entry (void * arg) {
    //int cnt = 0;

	for (;;) {
		//os_printf("task3 is running, %d\n", ++cnt);
        os_task_delay(2000);
	}
}

void task4_entry (void * arg) {
    //int cnt = 0;

	for (;;) {
        //os_printf("task4 is running, %d\n", ++cnt);
        os_task_delay(4000);
	}
}

void os_mem_simple_alloc_test (void) {
    os_err_t err;
    
	err = os_task_create_static(&task1, "task1", task1_entry, OS_NULL,
	                0, task1_stack, sizeof(task1_stack));
	os_assert(err == OS_ERR_OK);
	os_task_start(&task1);

    err = os_task_create_static(&task2, "task2", task2_entry, OS_NULL,
	                0, task2_stack, sizeof(task2_stack));
    os_assert(err == OS_ERR_OK);
	os_task_start(&task2);

	err = os_task_create_static(&task3, "task3", task3_entry, OS_NULL,
	                0, task3_stack, sizeof(task3_stack));
    os_assert(err == OS_ERR_OK);
	os_task_start(&task3);

    err = os_task_create_static(&task4, "task4", task4_entry, OS_NULL,
	                0, task4_stack, sizeof(task4_stack));
    os_assert(err == OS_ERR_OK);
	os_task_start(&task4);
}

