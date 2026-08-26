#include "os_task.h"
#include "os_plat.h"
#include "os_list.h"
#include "os_sched.h"
#include "os_men.h"
#include <string.h>

extern os_core_t os_core;

void os_task_delay(int ms)
{
    os_isr_status_t status = os_sched_isr_disable();
    os_task_t * self = os_task_self();
    os_sched_remove_ready(self);
    os_sched_set_delay(self,ms);

    os_sched_run();
    os_sched_isr_enable(status);
}

static os_err_t os_task_init(os_task_t * task,
                      const char *name,
                      taks_entry_t task_entry,
                      void *param,
                      int priority,
                      void *stack,       /* 内存块起始（低地址） */
                      int stack_size,    /* 内存块总大小（guard + 栈） */
                      int flag)    
{
    os_err_t err = os_task_create_static(task,name,task_entry,param,priority,stack,stack_size);
    task->task_flags = flag;
    return err;
}

void task_event_wait_init(task_wait_t * event_info,os_event_t * event,void *reason)
{
    event_info->event = event;
    event_info->err = OS_ERR_OK;
    event_info->reason = reason;
}

os_task_t * os_task_create(const char *name,
                           taks_entry_t task_entry,
                           void *param,
                           int priority,
                           int stack_size)
{
    os_param_failed(task_entry ==OS_NULL,OS_NULL);
    os_param_failed((priority < 0 )||(priority >= OS_TASK_PIRO_MAX),OS_NULL);
    os_param_failed(stack_size < sizeof(os_task_t),OS_NULL);

    uint8_t * stack_start =0;
    os_task_t * task = os_mem_malloc(sizeof(os_task_t));

    if(task == OS_NULL)
    {
        os_dbg("error task  malloc failed");
        goto create_failed;
    }
    stack_size = stack_size & ~(sizeof(cpu_stack_t) -1);
    stack_start = os_mem_malloc(stack_size);
    if(stack_start == OS_NULL)
    {
        os_dbg("error stack  malloc failed");
        goto create_failed;
    }
    os_err_t err = os_task_init(task,name,task_entry,param,priority,stack_start,stack_size,OS_TASK_CREATE_MEM_FLAG);
    if(err < 0)
    {
        os_dbg("error task init failed");
        goto create_failed;
    }
    return task;

create_failed:
    os_men_free(task);
    if(stack_start !=OS_NULL)
    {
        os_men_free(stack_start);
    }
    return OS_NULL;
}


os_err_t os_task_create_static(os_task_t * task,
                      const char *name,
                      taks_entry_t task_entry,
                      void *param,
                      int priority,
                      void *stack,       /* 内存区起始地址 */
                      int stack_size)    /* 内存区总大小 */
{
    /* ============ 1. 任务名拷贝到任务结构体 ============ */
    if (name != OS_NULL) {
        size_t name_len = strlen(name);
        if (name_len >= OS_TASK_NAME_SIZE) {
            return OS_ERR_NAME;              /* 名字超长，返回错误 */
        }
        strncpy(task->Name, name, OS_TASK_NAME_SIZE - 1);
        task->Name[OS_TASK_NAME_SIZE - 1] = '\0';   /* 保证以 '\0' 结尾 */
    } else {
        task->Name[0] = '\0';                /* 名字为空时置空串 */
    }

    /* ============ 2. 计算有效栈区（始终执行，不依赖宏） ============ */
    stack_size = stack_size & ~(sizeof(cpu_stack_t) - 1);
    void * effective_stack = stack;
    int effective_size = stack_size;

#if OS_SCHED_STACK_CHECK && OS_STACK_GUARD_EN
    /* 启用 guard 时：扣除 guard 区域，剩余作为有效栈区 */
    if (stack_size <= OS_STACK_GUARD_SIZE) {
        return OS_ERR_STACK;
    }
    effective_size = stack_size - OS_STACK_GUARD_SIZE;
    effective_size &= ~(sizeof(cpu_stack_t) - 1);   /* 4字节对齐 */

    task->guard_start = stack;
    task->guard_size  = OS_STACK_GUARD_SIZE;
    memset(task->guard_start, OS_STACK_GUARD_FILL, task->guard_size);

    effective_stack = (uint8_t *)stack + OS_STACK_GUARD_SIZE;
#else
    /* 未启用 guard：直接用原始栈区，按4字节对齐 */
    effective_size &= ~(sizeof(cpu_stack_t) - 1);
#endif

    /* 始终记录任务栈信息（供内存释放/调试使用） */
    task->start_stack = effective_stack;
    task->stack_size  = effective_size;

#if OS_SCHED_STACK_CHECK
    /* 启用栈检测时，填充栈区用于水线扫描 */
    memset(task->start_stack, OS_TASK_STACK_FILL, task->stack_size);
#endif

    /* ============ 3. 上下文初始化（用原始stack和stack_size） ============ */
    os_task_ctx_init(task, stack, stack_size, task_entry, param);

    /* ============ 4. 任务控制块其他字段初始化 ============ */
    task->task_flags = 0;
    task->prio = priority;
#if OS_SCHED_RR_EN
    task->slice = OS_TASK_SLICE;
#endif
    task->delay_tick = 0;

    /* ============ 5. 链表节点初始化 ============ */
    os_list_item_init(&task->info_item, os_task_t, info_item);
    os_list_item_init(&task->ready_item, os_task_t, ready_item);
    os_list_item_init(&task->event_item, os_task_t, event_item);

    task_event_wait_init(&task->event_info, OS_NULL, OS_NULL);

    /* ============ 6. 加入系统任务链表 ============ */
    os_sched_add_new(task);

    return OS_ERR_OK;
}

void os_task_start(os_task_t * task)
{
    os_isr_status_t status = os_sched_isr_disable();
    os_sched_set_ready(task);
    os_sched_isr_enable(status);
}
#if OS_TASK_EXIT_EN
void os_task_exit()
{
    os_isr_status_t status = os_sched_isr_disable();

    os_task_t * self =os_task_self();
    os_sched_remove_ready(self);
    os_sched_remove_all_list(self);
    os_sched_set_delete(self);

    os_sched_run(); 

    os_sched_isr_enable(status);
    
    os_assert(0);
}
#endif


#if OS_TASK_SUSPEND_EN
    os_err_t os_task_suspend(os_task_t * task)
    {
        os_isr_status_t status = os_sched_isr_disable();
        if(task == OS_NULL)
        {
            task = os_task_self();
        }
        
        if(task->task_flags & OS_TASK_SUSPEND_FLAG)
        {
            os_sched_isr_enable(status);
            return os_ERR_STATE;
        }
        

        if(task == os_core.curr_task)
        {
            os_sched_remove_ready(task);
            os_sched_set_suspend(task);
            os_sched_run();
        }else
        {
            if(task->task_flags & OS_TASK_READY_FLAG)
            {
                os_sched_remove_ready(task);
            }

            if(task->task_flags & (OS_TASK_DELAY_FLAG ))
            {
                os_sched_remove_delay(task);
            }

            os_sched_set_suspend(task);
        }
        os_sched_isr_enable(status);
        return OS_ERR_OK;
        
    }

    os_err_t os_task_resume(os_task_t * task)
    {
        os_isr_status_t status = os_sched_isr_disable();
        os_param_failed(task == OS_NULL,OS_ERR_PARAM);
        if(task ->task_flags & OS_TASK_SUSPEND_FLAG)
        {
            os_sched_remove_suspend(task);
            os_sched_set_ready(task);
            os_sched_run();
        }else
        {
            return os_ERR_STATE;
        }
        os_sched_isr_enable(status);
        return OS_ERR_OK;
    }
#endif