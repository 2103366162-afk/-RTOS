#include "os_sys.h"
#include "beep.h"
#include "button.h"

static os_timer_t scan_timer;
// static os_timer_t t1;
// static os_timer_t t2;
// static os_timer_t t3;
// static os_timer_t t4;
// 周期性触发定时器
static void scan_timer_proc (os_timer_t * timer, void * arg) {
    static int called_cnt;

    //os_printf("%s is running, arg = %d, cnt = %d\n",
    //        timer->name, (int)arg, called_cnt);

    if (button_pressed(BUTTON_1)) {
        os_printf("button 1 pressed\r\n");
    }

    if (++called_cnt >= 10) {
        os_printf("timer delete\r\n");
        called_cnt =0;
        // 停止定时器但不删除
        os_timer_stop(timer);

		// 不能在这里直接删除，因为定时器还需要使用
		// os_timer_free(timer);
    }
}

// 一次性触发定时器
static void beep_timer_proc (os_timer_t * timer, void * arg) {
    os_printf("beep stop\r\n");
    //beep_stop();
}

void task_entry (void * arg) {
    // 创建定时器
    os_timer_t * beep_timer = os_timer_create("beep", 1000,
            beep_timer_proc, OS_NULL, OS_TIMER_ONE_SHOT);

    for (;;) {
        // 按键按钮下，重新触发一次
        if (button_pressed(BUTTON_0)) {
            if (!os_timer_get_state_started(beep_timer)) {
                //beep_play(BEEP_WARNING);
                os_timer_start(beep_timer);
            }

            // 等待按键释放
            while (button_pressed(BUTTON_0)) {
                os_task_delay(10);
            }
        }
    }
}

void os_timer_simple_test (void) {
    beep_init();
    button_init();

    int arg = 0x10;
    os_timer_create_static(&scan_timer, "scan", 1000, scan_timer_proc, &arg, OS_TIMER_PERIOD);
     os_timer_start(&scan_timer);

    // os_timer_create_static(&t1, "t1", 100, scan_timer_proc, &arg, OS_TIMER_PERIOD);
    // os_timer_start(&t1);

    // os_timer_create_static(&t2, "t2", 1000, scan_timer_proc, &arg, OS_TIMER_PERIOD);
    // os_timer_start(&t2);

    // os_timer_create_static(&t3, "t3", 200, scan_timer_proc, &arg, OS_TIMER_PERIOD);
    // os_timer_start(&t3);

    // os_timer_create_static(&t4, "t4", 2000, scan_timer_proc, &arg, OS_TIMER_PERIOD);
    // os_timer_start(&t4);

    os_task_t * task = os_task_create("task0", task_entry, OS_NULL, 0, 1024);
    os_task_start(task);
}
