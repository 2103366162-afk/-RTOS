
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

// void task1_entry (void * arg) {
//     int cnt = 0;
    
//     led_on(LED0);
// 	for (;;) {
// 	    // 增加上锁之后，将不允许任务切换，但是仍然可以触发中断
// 	    // 注意，不能在上锁之后执行任何让任务暂停运行的操作，如延时】
// 	    os_sched_lock();
// 		os_printf("task1 is running, %d\n", ++cnt);
// 		led_toggle(LED0);
// 		os_sched_unlock();
//         //os_task_sleep(500);
// 	}
// }

// void task2_entry (void * arg) {
//     int cnt = 0;

//     led_on(LED1);
// 	for (;;) {
// 	    os_sched_lock();
//         led_toggle(LED1);
// 		os_printf("task2 is running, %d\n", ++cnt);
// 		os_sched_unlock();
//         //os_task_sleep(1000);
// 	}
// }

// void task3_entry (void * arg) {
//     int cnt = 0;

//     led_on(LED2);
// 	for (;;) {
// 	    os_sched_lock();
//         led_toggle(LED2);
// 		os_printf("task3 is running, %d\n", ++cnt);
// 		os_sched_unlock();
//         //os_task_sleep(2000);
// 	}
// }

// void task4_entry (void * arg) {
//     int cnt = 0;

//     led_on(LED3);
// 	for (;;) {
// 	    os_sched_lock();
//         led_toggle(LED3);
// 		os_printf("task4 is running, %d\n", ++cnt);
// 		os_sched_unlock();
//         //os_task_sleep(4000);
// 	}
// }

// void os_task_sleep_test (void) {
//     os_err_t err;
    
//     led_init();

// 	err = os_task_init(&task1, "task1", task1_entry, OS_NULL,
// 	                0, task1_stack, sizeof(task1_stack));
// 	os_assert(err == OS_ERR_OK);
// 	os_task_start(&task1);

//     err = os_task_init(&task2, "task2", task2_entry, OS_NULL,
// 	                0, task2_stack, sizeof(task2_stack));
//     os_assert(err == OS_ERR_OK);
// 	os_task_start(&task2);

// 	err = os_task_init(&task3, "task3", task3_entry, OS_NULL,
// 	                0, task3_stack, sizeof(task3_stack));
//     os_assert(err == OS_ERR_OK);
// 	os_task_start(&task3);

//     err = os_task_init(&task4, "task4", task4_entry, OS_NULL,
// 	                0, task4_stack, sizeof(task4_stack));
//     os_assert(err == OS_ERR_OK);
// 	os_task_start(&task4);
// }

