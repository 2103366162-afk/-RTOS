
// #include "os_sys.h"
// #include "button.h"
// #include "led.h"

// static cpu_stack_t task1_stack[200];
// static cpu_stack_t task2_stack[200];
// static cpu_stack_t task3_stack[200];
// static cpu_stack_t task4_stack[200];
// static os_task_t task1;
// static os_task_t task2;
// static os_task_t task3;
// static os_task_t task4;

// void task1_entry (void * arg) {
// 	int count = 0;

// 	for (;;) {
// 	    led_toggle(LED0);
// 		os_printf("task1 count = %d\n", count++);
//         os_task_suspend(os_task_self());
// 	}
// }

// void task2_entry (void * arg) {
//     int count = 0;

// 	for (;;) {
//         led_toggle(LED1);
//         os_printf("task2 count = %d\n", count++);
//         os_task_delay(1000);
// 		os_task_resume(&task1);
// 	}
// }

// void task3_entry (void * arg) {
//     int count = 0;

// 	for (;;) {
//         os_task_suspend(os_task_self());
//         os_printf("task3 count = %d\n", count++);
// 	}
// }

// void task4_entry (void * arg) {
// 	for (;;) {
// 	    if (button_pressed(BUTTON_0)) {
// 	        os_task_resume(&task3);
// 	    }
// 	}
// }

// void os_task_suspend_test (void) {
//     led_init();
//     button_init();

// 	os_task_init(&task1, "task1", task1_entry, OS_NULL, 0, task1_stack, sizeof(task1_stack));
// 	os_task_start(&task1);

// 	os_task_init(&task2, "task2", task2_entry, OS_NULL, 0, task2_stack, sizeof(task2_stack));
// 	os_task_start(&task2);

// 	os_task_init(&task3, "task3", task3_entry, OS_NULL, 1, task3_stack, sizeof(task3_stack));
// 	os_task_start(&task3);

// 	os_task_init(&task4, "task4", task4_entry, OS_NULL, 2, task4_stack, sizeof(task4_stack));
// 	os_task_start(&task4);

//     os_sys_start();
// }

