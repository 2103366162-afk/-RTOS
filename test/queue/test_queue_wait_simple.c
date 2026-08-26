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

typedef struct button_msg_t {
	button_t button;
}button_msg_t;

#define MSG_CNT		10
static button_msg_t msg_buf[MSG_CNT];
static os_queue_t queue;

// static os_queue_t * queue;


void task1_entry (void * arg) {

	for (;;) {
        button_msg_t read_msg;
        os_queue_show_status(&queue);
        os_queue_read(&queue, 0, 0, &read_msg);
		os_printf("button %d pressed\r\n", read_msg.button);
		os_task_delay(500);
	}
}

void task2_entry (void * arg) {
    
    for (;;) 
    {
        // if (button_pressed(BUTTON_0)) {
        //     button_msg_t msg;

        //     msg.button = BUTTON_0;
        //     os_queue_write(&queue, 0, OS_QUEUE_RELESE_UPGENT, &msg);
        // }
        if (button_pressed(BUTTON_1)) {
            button_msg_t msg;

            msg.button = BUTTON_1;
            os_queue_write(&queue, 0, OS_QUEUE_RELESE_UPGENT, &msg);
        }
        if (button_pressed(BUTTON_2)) {
            button_msg_t msg;

            msg.button = BUTTON_2;
            os_queue_write(&queue, 0, OS_QUEUE_RELESE_UPGENT, &msg);
        }
        if (button_pressed(BUTTON_3)) {
            button_msg_t msg;

            msg.button = BUTTON_3;
            os_queue_write(&queue, 0, OS_QUEUE_RELESE_UPGENT, &msg);
        }
        os_task_delay(100);
    }
}

void task3_entry (void * arg) {
    //int cnt = 0;

	for (;;) {
		//os_printf("task3 is running, %d\n", ++cnt);
        os_task_delay(1000);
	}
}

void task4_entry (void * arg) {
    //int cnt = 0;

	for (;;) {
        //os_printf("task4 is running, %d\n", ++cnt);
        os_task_delay(1000);
	}
}

 void os_queue_wait_simple_test (void) {
    button_init();

    os_queue_create_static(&queue, msg_buf, sizeof(button_msg_t), MSG_CNT);

    // queue = os_queue_create(sizeof(button_msg_t), MSG_CNT);

    os_err_t err;
	err = os_task_create_static(&task1, "task1", task1_entry, OS_NULL,
	                0, task1_stack, sizeof(task1_stack));
	os_assert(err == OS_ERR_OK);
	os_task_start(&task1);

    err = os_task_create_static(&task2, "task2", task2_entry, OS_NULL,
	                1, task2_stack, sizeof(task2_stack));
    os_assert(err == OS_ERR_OK);
	os_task_start(&task2);

	err = os_task_create_static(&task3, "task3", task3_entry, OS_NULL,
	                2, task3_stack, sizeof(task3_stack));
    os_assert(err == OS_ERR_OK);
	os_task_start(&task3);

    err = os_task_create_static(&task4, "task4", task4_entry, OS_NULL,
	                2, task4_stack, sizeof(task4_stack));
    os_assert(err == OS_ERR_OK);
	os_task_start(&task4);
}


