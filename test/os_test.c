#include "os_test.h"
#include "os_sys.h"

void os_test()
{
    USART_Printf_Init(115200);
    
    os_sys_init();
    //os_list_test_simple();
    //os_task_yield_test();
    //os_task_sleep_test();
    //os_task_prio_test();
    //os_task_exit_test();
    //os_task_suspend_test() ;
    //os_mem_simple_alloc_test();
    //os_task_create_test();
    //os_task_event_sync_test();
    //os_mblock_wait_release_test();
    //os_mutex_lock_simple_test();
    //os_sem_wait_simple_test();
    //os_queue_wait_simple_test();
    //os_eflags_wait_button_test ();
    //os_timer_simple_test ();
    //bench_test_init();
    stress_test_init();
    os_sys_start();
}
