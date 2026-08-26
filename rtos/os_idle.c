#include "os_sys.h"
extern os_core_t os_core;

#if OS_SCHED_STACK_CHECK && OS_STACK_GUARD_EN
static uint8_t idle_mem[OS_STACK_GUARD_SIZE + OS_TASK_IDLE_STACK_TASK];
#else 
static uint8_t idle_mem[OS_TASK_IDLE_STACK_TASK];
#endif

void idle_task_entry(void *param)
{
    for(;;)
    {
#if OS_TASK_EXIT_EN
        os_task_t * delete_task = os_sched_remove_delete();
        while(delete_task != OS_NULL)
        {
            os_dbg("delete task: %s\r\n",delete_task->Name);

            //调用自己实现的c free函数
            
            delete_task = os_sched_remove_delete();
            if(delete_task ->task_flags & OS_TASK_CREATE_MEM_FLAG)
            {
                os_men_free(delete_task->start_stack);
                os_men_free(delete_task);
            }
        }
#endif
    }
}

os_err_t os_task_idle_init(void)
{
    os_err_t err = os_task_create_static(&os_core.idle_task,
                                        "idle_task",
                                        idle_task_entry,
                                        OS_NULL,
                                        OS_TASK_IDLE_PRIO,
                                        idle_mem,
                                        sizeof(idle_mem));

    if(err<0)
    {
        os_dbg("idle task init error");
        return err;
    }
    os_task_start(&os_core.idle_task);
    return err;
}