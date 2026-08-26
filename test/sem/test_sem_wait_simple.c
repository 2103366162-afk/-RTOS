
#include "os_sys.h"
#include "button.h"

static os_sem_t * button0_sem;

static cpu_stack_t task1_stack[200];
static cpu_stack_t task2_stack[200];
static cpu_stack_t task3_stack[200];
static cpu_stack_t task4_stack[200];
static os_task_t task1;
static os_task_t task2;
static os_task_t task3;
static os_task_t task4;

void task1_entry (void * arg) {
	int count = 0;
	
	for (;;) {
	    if (os_sem_cnt(button0_sem) == 0) {
	        count = 0;
	    }

		// 等待第2个任务发信号量号才进行翻转
		os_sem_take(button0_sem, 0);
		int wait_cnt = os_sem_cnt(button0_sem);
		os_printf("task1 sem take: %d\r\n", wait_cnt);
		os_task_delay(200);
	}
}

void task2_entry (void * arg) {
	for (;;) {
	    if (button_pressed(BUTTON_0)) {
	        // 发信号量通知task1可以翻转信号
	        int wait_cnt = os_sem_wait_tasks_cnt(button0_sem);
	        os_sem_release(button0_sem);
	        os_printf("task2 sem give: %d, wait: %d\r\n",
	                os_sem_cnt(button0_sem), wait_cnt);

	        // 避免太快发信号
	        os_task_delay(100);
	    }

	}
}

void task3_entry (void * arg) {
	//int count = 0;

	for (;;) {
	    //os_printf("task3 is running, %d\n", count++);
        os_task_delay(1000);
	}
}

void task4_entry (void * arg) {
	//int count = 0;

	for (;;) {
	    //os_printf("task4 is running, %d\n", count++);
        os_task_delay(1000);
	}
}

void os_sem_wait_simple_test (void) {
    button_init();

    button0_sem = os_sem_create(0, 100);

	os_task_create_static(&task1, "task1", task1_entry, OS_NULL, 0, task1_stack, sizeof(task1_stack));
	os_task_start(&task1);

	os_task_create_static(&task2, "task2", task2_entry, OS_NULL, 0, task2_stack, sizeof(task2_stack));
	os_task_start(&task2);

	os_task_create_static(&task3, "task3", task3_entry, OS_NULL, 1, task3_stack, sizeof(task3_stack));
	os_task_start(&task3);

	os_task_create_static(&task4, "task4", task4_entry, OS_NULL, 1, task4_stack, sizeof(task4_stack));
	os_task_start(&task4);
}

