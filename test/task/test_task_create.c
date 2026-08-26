#include "os_task.h"

os_task_t * task1;
os_task_t * task2;
os_task_t * task3;
os_task_t * task4;

void task1_entry (void * arg) {
	int count = 0;

	for (;;) {
		os_printf("task1 count = %d\n", count++);
        os_task_delay(100);
	}
}

void task2_entry (void * arg) {
    int count = 0;

	for (;;) {
        os_printf("task2 count = %d\n", count++);
        os_task_delay(100);
	}
}

void task3_entry (void * arg) {
    int count = 0;

	for (;;) {
        os_printf("task3 count = %d\n", count++);
        os_task_delay(100);
	}
}

void task4_entry (void * arg) {
    int count = 0;

	for (;;) {
        os_printf("task4 count = %d\n", count++);
        os_task_delay(100);
	}
}

void os_task_create_test (void) 
{
    task1 = os_task_create("task1", task1_entry, OS_NULL, 0, 1000);
    os_task_start(task1);

    task2 = os_task_create("task2", task2_entry, OS_NULL, 0, 1000);
    os_task_start(task2);

    task3 = os_task_create("task3", task3_entry, OS_NULL, 0, 1000);
    os_task_start(task3);

    task4 = os_task_create("task4", task4_entry, OS_NULL, 0, 1000);
    os_task_start(task4);
}

