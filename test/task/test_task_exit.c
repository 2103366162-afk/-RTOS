// #include "os_sys.h"
// #include "button.h"

// static cpu_stack_t task1_stack[400];
// static cpu_stack_t task2_stack[400];
// static cpu_stack_t task3_stack[400];
// static uint8_t task4_stack[800];
// static os_task_t task1;
// static os_task_t task2;
// static os_task_t task3;
// static os_task_t task4;

// void task1_entry (void * arg) {
// 	int count = 0;

// 	for (;;) {
// 		os_task_delay(1000);
//         os_printf("task1 count = %d\r\n", count++);
//         if (button_pressed(BUTTON_0)) {
//             os_printf("task 1 exit\r\n");
//             os_task_exit();
//         }
// 	}
// }

// void task2_entry (void * arg) {
//     int count = 0;

// 	for (;;) {
//         os_task_delay(1000);
//         os_printf("task2 count = %d\r\n", count++);
// 		if (button_pressed(BUTTON_1)) {
// 		    os_printf("task 2 exit\r\n");
// 			os_task_exit();
// 		}
// 	}
// }

// void task3_entry (void * arg) {
//     int count = 0;

// 	for (;;) {
//         os_task_delay(1000);
//         os_printf("task3 count = %d\r\n", count++);
//         if (button_pressed(BUTTON_2)) {
//             os_printf("task 3 exit\r\n");
//             return;
//         }
// 	}
// }

// void task4_entry (void * arg) {
//     int count = 0;

// 	for (;;) {
//         os_task_delay(1000);
//         os_printf("task4 count = %d\r\n", count++);
//         if (button_pressed(BUTTON_3)) {
//             os_printf("task 4 exit\r\n");
//             os_task_exit();
//         }
// 	}
// }

// void os_task_exit_test (void) {
//     button_init();

// 	os_task_init(&task1, "task1", task1_entry, OS_NULL, 1, task1_stack, sizeof(task1_stack));
// 	os_task_start(&task1);

// 	os_task_init(&task2, "task2", task2_entry, OS_NULL, 1, task2_stack, sizeof(task2_stack));
// 	os_task_start(&task2);

// 	os_task_init(&task3, "task3", task3_entry, OS_NULL, 1, task3_stack, sizeof(task3_stack));
// 	os_task_start(&task3);

// 	os_task_init(&task4, "task4", task4_entry, OS_NULL, 0, task4_stack, sizeof(task4_stack));
// 	os_task_start(&task4);
// }

