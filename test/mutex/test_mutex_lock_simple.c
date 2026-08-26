
#include "os_sys.h"

static cpu_stack_t task1_stack[200];
static cpu_stack_t task2_stack[200];
static cpu_stack_t task3_stack[200];
static cpu_stack_t task4_stack[200];
static os_task_t task1;
static os_task_t task2;
static os_task_t task3;
static os_task_t task4;

#define LOOP_CNT 10
#define ADD_DEC_CNT 5000000  // 如果效果不明显，则将值改大一些

os_mutex_t *mutex;
volatile int count = 0;
#define USE_MUTEX_LOCK 1  // 是否使用互斥信号量

void task1_entry (void *arg) {
    os_task_delay(2000);
#if USE_MUTEX_LOCK
    os_mutex_lock (mutex, 0);
#endif
    for (int j = 0; j < ADD_DEC_CNT; j++) {
        count++;
    }
#if USE_MUTEX_LOCK
    os_printf ("mutex owner: %s, lock cnt: %d, wait task: %d, prio: %d\r\n",
               os_mutex_owner (mutex)->Name, os_mutex_lock_cnt (mutex),
               os_mutex_wait_task_cnt (mutex), os_task_self()->prio);

    os_mutex_unlock (mutex);
#endif
    os_printf ("task1: count=%d\r\n", count);

    for (;;) {
        
    }
}

void task2_entry (void *arg) {
    os_task_delay(800);
    for (;;) {
    //os_task_delay(200);
    }
}

void task3_entry (void *arg) {

    #if USE_MUTEX_LOCK
    os_mutex_lock (mutex, 0);
#endif
    for (int j = 0; j < ADD_DEC_CNT; j++) {
        count--;
    }
#if USE_MUTEX_LOCK
    os_printf ("mutex owner: %s, lock cnt: %d, wait task: %d, prio: %d\r\n",
               os_mutex_owner (mutex)->Name, os_mutex_lock_cnt (mutex),
               os_mutex_wait_task_cnt (mutex), os_task_self()->prio);
    os_task_delay(1000);
    os_mutex_unlock (mutex);
#endif
    os_printf ("task3: count=%d\r\n", count);

    // 延迟一段时间后再释放
    os_task_delay (2000);
    os_mutex_free (mutex);
    for (;;) {
        // os_printf("task3 is running, %d\n", ++cnt);
        os_task_delay (1000);
    }
}

void task4_entry (void *arg) {
    os_task_delay(1000);
    for (;;) {

    }
}

void os_mutex_lock_simple_test (void) {
    mutex = os_mutex_create();

    os_err_t err;
    err = os_task_create_static (&task1, "task1", task1_entry, OS_NULL,
                                 0, task1_stack, sizeof (task1_stack));
    os_assert (err == OS_ERR_OK);
    os_task_start (&task1);

    err = os_task_create_static (&task2, "task2", task2_entry, OS_NULL,
                                 1, task2_stack, sizeof (task2_stack));
    os_assert (err == OS_ERR_OK);
    os_task_start (&task2);

    err = os_task_create_static (&task3, "task3", task3_entry, OS_NULL,
                                 2, task3_stack, sizeof (task3_stack));
    os_assert (err == OS_ERR_OK);
    os_task_start (&task3);

    err = os_task_create_static (&task4, "task4", task4_entry, OS_NULL,
                                 1, task4_stack, sizeof (task4_stack));
    os_assert (err == OS_ERR_OK);
    os_task_start (&task4);
}
