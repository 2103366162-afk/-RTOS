
#include "os_sys.h"
#include "button.h"

static cpu_stack_t task1_stack[200];
static cpu_stack_t task2_stack[200];
static cpu_stack_t task3_stack[200];
static cpu_stack_t task4_stack[200];
static os_task_t task1;
static os_task_t task2;
static os_task_t task3;
static os_task_t task4;

static os_eflags_t eflags;

#define BUTTON_ALL_FLAGS		0xF

void task1_entry (void * arg) {
    os_flags_t flags = 0;

	for (;;) {
	    flags = os_eflags_wait_bits(&eflags, 0,
	            OS_EFLAGS_SET_ALL | OS_EFLAGS_EXIT_CLEAR,
	            BUTTON_ALL_FLAGS, OS_NULL);
	    os_printf("eflags: %d\r\n", flags);

	    if (flags & (1 << 0)) {
	        os_printf("button 0 pressed.\r\n");
	    }
        if (flags & (1 << 1)) {
            os_printf("button 1 pressed.\r\n");
        }
        if (flags & (1 << 2)) {
            os_printf("button 2 pressed.\r\n");
        }
        if (flags & (1 << 3)) {
            os_printf("button 3 pressed.\r\n");
        }

	}
}

void task2_entry (void * arg) {
    for (;;) {
        if (button_pressed(BUTTON_0)) {
            os_eflags_set_bits(&eflags, OS_EFLAGS_SET, BUTTON_ALL_FLAGS);
        }

        if (button_pressed(BUTTON_1)) {
            os_eflags_set_bits(&eflags, OS_EFLAGS_SET, 1 << 1);
        }

        if (button_pressed(BUTTON_2)) {
            os_eflags_set_bits(&eflags, OS_EFLAGS_SET, 1 << 2);
        }

        if (button_pressed(BUTTON_3)) {
            os_eflags_set_bits(&eflags, OS_EFLAGS_SET, 1 << 3);
        }

        os_task_delay(1000);
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

void os_eflags_wait_button_test (void) {
    button_init();

	os_eflags_create_static(&eflags, 0xFFFFFFFF);

	os_task_create_static(&task1, "task1", task1_entry, OS_NULL, 0, task1_stack, sizeof(task1_stack));
	os_task_start(&task1);

	os_task_create_static(&task2, "task2", task2_entry, OS_NULL, 1, task2_stack, sizeof(task2_stack));
	os_task_start(&task2);

	os_task_create_static(&task3, "task3", task3_entry, OS_NULL, 2, task3_stack, sizeof(task3_stack));
	os_task_start(&task3);

	os_task_create_static(&task4, "task4", task4_entry, OS_NULL, 2, task4_stack, sizeof(task4_stack));
	os_task_start(&task4);
}

