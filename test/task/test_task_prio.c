
// #include "os_sys.h"
// #include "led.h"

// static cpu_stack_t task1_stack[200];
// static cpu_stack_t task2_stack[200];
// static cpu_stack_t task3_stack[200];
// static cpu_stack_t task4_stack[200];
// static os_task_t task1;
// static os_task_t task2;
// static os_task_t task3;
// static os_task_t task4;


// void delay (void) {
//     for (int i = 0; i < 10; i++) {
//         for (int j = 0; j < 10000; j++) {

//         }
//     }
// }

// // 只能任务1和2才能运行
// static void task1_entry (void * arg) {
// 	int count = 0;
	
// 	for (;;) {
// 		os_printf("task1 count = %d\r\n", count++);
// 		led_toggle(LED0);
//         os_task_delay(100);
//         //os_sched_yield();
// 	}
// }

// static void task2_entry (void * arg) {
//     int count = 0;
	
// 	for (;;) {
//         os_printf("task2 count = %d\r\n", count++);
//         led_toggle(LED1);
//         os_task_delay(100);
//         os_sched_yield();
// 	}
// }

// static void task3_entry (void * arg) {
//     int count = 0;
	
// 	for (;;) {
//         os_printf("task3 count = %d\r\n", count++);
//         led_toggle(LED2);
//         os_task_delay(100);
//         os_sched_yield();
// 	}
// }

// static void task4_entry (void * arg) {
//     int count = 0;
	
// 	for (;;) {
//         os_printf("task4 count = %d\r\n",count++);
//         led_toggle(LED3);
//         os_task_delay(100);
//         os_sched_yield();
// 	}
// }

// void os_task_prio_test (void) {
//     led_init();

  
// 	os_err_t err = os_task_init(&task1, "task1", task1_entry, OS_NULL,
// 	                3, task1_stack, sizeof(task1_stack));
// 	os_assert(err == OS_ERR_OK);
// 	os_task_start(&task1);
	
// 	err = os_task_init(&task2, "task2", task2_entry, OS_NULL,
// 	                2, task2_stack, sizeof(task2_stack));
//     os_assert(err == OS_ERR_OK);
// 	os_task_start(&task2);
	
// 	// 任务3和4同优先级
// 	err = os_task_init(&task3, "task3", task3_entry, OS_NULL,
// 	                0, task3_stack, sizeof(task3_stack));
//     os_assert(err == OS_ERR_OK);
// 	os_task_start(&task3);
	
// 	err = os_task_init(&task4, "task4", task4_entry, OS_NULL,
// 	                1, task4_stack, sizeof(task4_stack));
//     os_assert(err == OS_ERR_OK);
// 	os_task_start(&task4);
// }

