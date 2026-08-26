#include "os_event.h"
#include "os_dbg.h"
#include "os_err.h"
#include "os_task.h"
 #include "os_sched.h"

extern os_core_t os_core;

os_err_t os_event_init (os_event_t * event, os_event_type_t type, int flags)
{
    os_assert(event != OS_NULL);
    os_assert((type >= OS_EVENT_TYPE_INVALID) && (type <= OS_EVENT_TYPE_MAX));

    event->type = type;
    event->flag = flags;
    os_list_init(&event->wait_list,os_task_t,event_item);
    return OS_ERR_OK;
}

os_err_t os_event_uninit (os_event_t * event)
{
    os_assert(event != OS_NULL);
    os_event_notify_all(event,OS_NULL,OS_ERR_REMOVE);
    return OS_ERR_OK;
}

void os_event_insert_task(os_event_t *event, os_task_t *task)
{
    os_task_t *pre = OS_NULL;   // 记录上一个节点

    task->task_flags |= OS_TASK_EVENT_WAIT;
    task->event_info.event = event;

    os_list_for_each(next, os_task_t, &event->wait_list) {
        if (next->prio < task->prio) {
            break;               // 找到第一个严格低优先级的节点
        }
        pre = next;              // 否则 pre 一直指向当前节点，作为前驱
    }

    if (next == OS_NULL) {
        // 队列为空，或者所有节点优先级都不低于 task
        os_list_insert_last(&event->wait_list, task);
    } else if (pre == OS_NULL) {
        // 第一个节点就比 task 低，插入队首
        os_list_insert_first(&event->wait_list, task);
    } else {
        // 插入到 pre 之后
        os_list_insert_after(&event->wait_list, pre, task);
    }
}

void os_event_remove_task(os_event_t *event,os_task_t *task)
{
    task->task_flags &= ~OS_TASK_EVENT_WAIT;
    task->event_info.event = OS_NULL;
    os_list_remove_item(&event->wait_list,task);
}

void os_event_wait (os_event_t * event, void * reason, int ms)
{
    os_assert(event != OS_NULL);

    os_task_t * self = os_task_self();

    task_event_wait_init(&self->event_info,event,reason);
    
    os_sched_remove_ready(self);
    if(ms>0)
    {
        os_sched_set_delay(self,ms);
    }
    os_event_insert_task(event,self);
}

struct _os_task_t * os_event_notify (os_event_t * event)
{
    os_assert(event != OS_NULL);
    os_task_t *task = (os_task_t *)os_list_first(&event->wait_list);
    if(task == OS_NULL)
    {
        return OS_NULL;
    }
    os_event_remove_task(event,task);
    if(task->task_flags & OS_TASK_DELAY_FLAG)
    {
        os_sched_remove_delay(task);
    }
    os_sched_set_ready(task);
    return task;
}

void os_event_notify_task (os_event_t * event, struct _os_task_t * task, void * reason, os_err_t err)
{
    os_assert(event != OS_NULL);
    os_assert(task != OS_NULL);
    os_assert(task->event_info.event == event);

    os_event_remove_task(event,task);

    if(task->task_flags & OS_TASK_DELAY_FLAG)
    {
        os_sched_remove_delay(task);
    }

    task_wait_t * wait = &task->event_info;
    wait->err = err;
    wait->reason = reason;
    os_sched_set_ready(task);
}

void os_event_notify_all (os_event_t * event, void * reason, os_err_t err)
{
    os_assert(event != OS_NULL);

    os_task_t *task;
    while((task  = (os_task_t *)os_list_first(&event->wait_list))!=OS_NULL)
    {   
        os_event_remove_task(event,task);

        if(task->task_flags & OS_TASK_DELAY_FLAG)
        {
            os_sched_remove_delay(task);
        }

        task_wait_t * wait = &task->event_info;
        wait->err = err;
        wait->reason = reason;
        
        os_sched_set_ready(task);
        
    }
}

int os_event_wait_cnt (os_event_t * event)
{
    os_assert (event!= OS_NULL);
    return os_list_count(&event->wait_list);
}


