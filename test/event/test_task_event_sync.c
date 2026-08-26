
#include "os_sys.h"
#include "led.h"
#include "button.h"
#include "os_event.h"

static os_event_t button_event;
    os_task_t * task1;
    os_task_t * task2;
    os_task_t * task3;
    os_task_t * task4;

void task1_entry (void * arg) {
	int count = 0;

	for (;;) {
	    // 等待事件通知，如果超过5秒钟则超时
	    os_sched_lock();
	    os_event_wait(&button_event, OS_NULL, 0);
        os_sched_unlock();

		os_printf("task1 count = %d\n", count++);
		led_toggle(LED0);

		//os_task_delay(100);
	}
}

void task2_entry (void * arg) {
    int count = 0;

	for (;;) {
        os_sched_lock();

        // 如果按下按键，则通知任务1
        if (button_pressed(BUTTON_0)) {
            os_printf("task2 count = %d\n", count++);
            led_toggle(LED1);

            // 通知任务1事件发生
            os_event_notify(&button_event);
        }

        os_sched_unlock();

        os_task_delay(100);
	}
}

void task3_entry (void * arg) {
    //int count = 0;

	for (;;) {
        //os_printf("task3 count = %d\n", count++);
        os_task_delay(100);
	}
}

void task4_entry (void * arg) {
    //int count = 0;

	for (;;) {
        //os_printf("task4 count = %d\n", count++);
        os_task_delay(100);
	}
}

void os_task_event_sync_test (void) {
    led_init();
    button_init();

    os_event_init(&button_event, OS_EVENT_TYPE_INVALID, 0);

    task1 = os_task_create("task1", task1_entry, OS_NULL, 0, 1000);
    os_task_start(task1);

    task2 = os_task_create("task2", task2_entry, OS_NULL, 0, 1000);
    os_task_start(task2);

    task3 = os_task_create("task3", task3_entry, OS_NULL, 1, 1000);
    os_task_start(task3);

    task4 = os_task_create("task4", task4_entry, OS_NULL, 1, 1000);
    os_task_start(task4);
}

