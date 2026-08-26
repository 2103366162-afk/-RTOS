#include "os_mutex.h"
#include "os_err.h"
#include "os_dbg.h"
#include "os_sched.h"
#include "os_event.h"

#if OS_MUTEX_EN

#if !OS_MUTEX_DBG_PRINT_EN
#undef os_dbg
#define os_dbg(fmt, ...) do {}while (0)
#endif

extern os_core_t os_core;

static os_err_t mutex_init(os_mutex_t * mutex,int flag)
{
    os_err_t err = os_event_init(&mutex->event,OS_EVENT_TYPE_MUTEX,flag);\
    if(err<0)
    {
        os_dbg("create mutex event init faied\r\n");
        return err;
    }
    mutex->owner = OS_NULL;
    mutex->locked_cnt = 0;
    mutex->raw_prio = OS_TASK_IDLE_PRIO;
    return OS_ERR_OK;
}

os_err_t os_mutex_create_static (os_mutex_t * mutex)
{
    os_param_failed(mutex == 0,OS_ERR_PARAM);

    return mutex_init(mutex,0);   
}

os_err_t os_mutex_uninit (os_mutex_t * mutex)
{
    os_param_failed(mutex == 0,OS_ERR_PARAM);

    if(mutex->owner != OS_NULL)
    {
        os_dbg("mutex uninit failed ,mutex is lock(owner != OS_NULL)");
        return OS_ERR_LOCKED;
    }

    os_isr_status_t status =  os_sched_isr_disable();
    int cnt = os_event_wait_cnt(&mutex->event);
    os_event_uninit(&mutex->event);
    if(cnt > 0)
    {
        os_sched_run();
    }
    os_sched_isr_enable(status);

    return OS_ERR_OK;
} 

os_mutex_t * os_mutex_create (void)
{
    /*分配mutex结构体*/
    os_mutex_t * mutex = os_mem_malloc(sizeof(os_mutex_t));
    if(mutex == OS_NULL)
    {
        os_dbg("error:mutex mem_malloc failed");
        return  OS_NULL;
    }

    os_err_t err = mutex_init(mutex,OS_FLAG_MEM_HEAP);
    if(err<0)
    {
        os_dbg("error:mutex init failed");
        os_men_free(mutex);
        return OS_NULL;
    }

    return  mutex;
}

os_err_t os_mutex_free (os_mutex_t * mutex)
{
    os_param_failed(mutex == OS_NULL,OS_ERR_PARAM);

    os_err_t err = os_mutex_uninit(mutex);
    if(err<0)
    {
        os_dbg("mutex uninit failed");
        return err;
    }

    if(mutex->event.flag & OS_FLAG_MEM_HEAP)
    {
        os_men_free(mutex);
    }

    return  OS_ERR_OK;
}


os_err_t os_mutex_lock (os_mutex_t * mutex, int ms)
{
    os_param_failed(mutex == OS_NULL,OS_ERR_PARAM);
    os_isr_status_t status =  os_sched_isr_disable();

    /*未上锁,此时把mutex占为所有*/
    if(mutex->owner == OS_NULL) 
    {
        mutex->owner = os_task_self();
        mutex->raw_prio = mutex->owner->prio;
        mutex->locked_cnt = 1 ;
        os_sched_isr_enable(status);
        return OS_ERR_OK;
    }

    /*已经上锁, 且上锁者是任务本身，即嵌套上锁*/ 
    if(mutex->locked_cnt != 0 && mutex->owner == os_task_self())
    {
        mutex->locked_cnt ++;
        os_dbg("[mutex_lock]:mutex is locked and lock is self,cnt: %d",mutex->locked_cnt);
        os_sched_isr_enable(status);
        return OS_ERR_OK;
    }

    /*已经上锁, 不延时等待或在中断中误调用，直接返回*/ 
    if((ms < 0) || (os_core.os_sched_in_isr_flag > 0))
    {   
        os_sched_isr_enable(status);
        os_dbg("mutex is locked and no wait");
        return OS_ERR_LOCKED; 
    } 

    /*已经上锁, 且进行延时等待*/   
    os_task_t * self = os_task_self();

    /*优先级翻转问题 把拥有者提高优先级*/
    if(mutex->owner->prio > self->prio)
    {
        os_dbg("mutex prio change: task %s prio from %d to %d",mutex->owner->Name,mutex->owner->prio,self->prio);

        if(mutex->owner->task_flags & OS_TASK_READY_FLAG)
        {
            os_sched_remove_ready(mutex->owner);
            mutex->owner->prio = self->prio;
            os_sched_set_ready(mutex->owner);
        }else if(mutex->owner->task_flags & OS_TASK_EVENT_WAIT)
        {
            os_event_t * event = mutex->owner->event_info.event;
            os_event_remove_task(event,mutex->owner);
            mutex->owner->prio = self->prio;
            os_event_insert_task(event,mutex->owner);
        }else
        {
            mutex->owner->prio = self->prio;
        }        
    }
    self->event_info.err = OS_ERR_LOCKED;
    os_event_wait(&mutex->event,OS_NULL,ms);
    os_sched_isr_enable(status);
    os_sched_run();     
    return self->event_info.err;

}

os_err_t os_mutex_unlock (os_mutex_t * mutex)
{
    os_param_failed(mutex == OS_NULL,OS_ERR_PARAM);
    os_isr_status_t status =  os_sched_isr_disable();
    os_task_t * self = os_task_self();
    
    /*要解锁，互斥锁应该处于锁定状态*/
    if(mutex->owner == OS_NULL)
    {
        os_dbg("mutex is no locked");
        os_sched_isr_enable(status);
        return OS_ERR_UNLOCKED;
    }

    /*解的锁得是自己所占用的，不能去解别人的锁*/
    os_task_t * mutex_owner_task = mutex->owner;
    if(mutex_owner_task != self)
    {
        os_dbg("mutex's owner is not self");
        os_sched_isr_enable(status);
        return OS_ERR_OWNER;
    }

    /*锁嵌套的情况*/
    if(--mutex->locked_cnt > 0)
    {
        os_dbg("[mutex_unlock]:mutex is locked and lock is self,cnt: %d",mutex->locked_cnt);
        os_sched_isr_enable(status);
        return OS_ERR_OK;
    }

    /*优先级恢复*/
    if(mutex->raw_prio != self->prio)
    {
        os_dbg("mutex prio restore,task %s ,prio %d from %d",mutex->owner->Name, mutex->owner->prio,mutex->raw_prio);
        os_sched_remove_ready(mutex->owner);
        mutex->owner->prio = mutex->raw_prio;
        os_sched_set_ready(mutex->owner);
    }

    /*有任务在等待，唤醒它*/
    int cnt = os_event_wait_cnt(&mutex->event);
    if(cnt > 0 )
    {
        os_task_t * task = os_event_notify(&mutex->event);
        task->event_info.err = OS_ERR_OK;
        task->event_info.reason = OS_NULL;
        mutex->owner = task;
        mutex->raw_prio = task->prio;
        mutex->locked_cnt = 1;
        os_sched_isr_enable(status);
        os_sched_run();
        return OS_ERR_OK;
    }else /*无任务在等待*/
    {
        mutex->owner = OS_NULL;
        os_sched_isr_enable(status);
        return OS_ERR_OK;
    }
}

#if OS_MUTEX_INFO_EN

uint16_t os_mutex_lock_cnt (os_mutex_t * mutex)
{
    os_param_failed(mutex == OS_NULL,OS_ERR_PARAM);
    
    os_isr_status_t status =  os_sched_isr_disable();
    uint16_t cnt = mutex->locked_cnt;
    os_sched_isr_enable(status);
    return cnt;
}

uint16_t os_mutex_wait_task_cnt (os_mutex_t * mutex)
{
    os_param_failed(mutex == OS_NULL,OS_ERR_PARAM);

    os_isr_status_t status =  os_sched_isr_disable();
    uint16_t task_cnt = os_event_wait_cnt(&mutex->event);
    os_sched_isr_enable(status);
    return task_cnt;
}

os_task_t * os_mutex_owner (os_mutex_t * mutex)
{
    os_param_failed(mutex == OS_NULL,OS_NULL);

    os_isr_status_t status =  os_sched_isr_disable();
    os_task_t * task = mutex->owner;
    os_sched_isr_enable(status);
    return task;
}

#endif
#endif