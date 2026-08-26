#include "os_task.h"
#include "os_sched.h"
#include "os_timer.h"
#include "os_men.h"
#include "os_mutex.h"
#include <string.h>

#if OS_TIMER_EN

/*
 * 定时器控制块结构体
 * 将原本独立的静态全局变量放入同一个结构体，
 * 便于统一管理，避免散落的全局变量。
 */
typedef struct {
    os_list_t timer_list;          /* 活跃定时器链表 */
    os_list_t timer_timeout_list;  /* 超时定时器链表 */
    os_event_t timer_list_event;   /* 定时器链表事件 */
    os_mutex_t os_timer_mutex;     /* 保护定时器链表的互斥量 */
} os_timer_control_t;

/* 定时器控制块实例（替代原来的多个全局变量） */
static os_timer_control_t os_timer_control;

/* 初始化定时器控制块 */
static void os_timer_control_init(void)
{
    os_list_init(&os_timer_control.timer_list, os_timer_t, item);
    os_list_init(&os_timer_control.timer_timeout_list, os_timer_t, item);
    os_event_init(&os_timer_control.timer_list_event, OS_EVENT_TYPE_TIMER, 0);
    os_mutex_create_static(&os_timer_control.os_timer_mutex);
}

/* 进入临界区（加锁） */
static void timer_list_start_protect(void)
{
    os_mutex_lock(&os_timer_control.os_timer_mutex, 0);
}

/* 退出临界区（解锁） */
static void timer_list_end_protect(void)
{
    os_mutex_unlock(&os_timer_control.os_timer_mutex);
}

/* 将定时器按剩余时间插入有序链表（时间差值保持正确） */
static void insert_timer_list(os_timer_t *timer)
{
    os_timer_t *pre = OS_NULL;
    os_list_for_each(curr, os_timer_t, &os_timer_control.timer_list)
    {
        if (curr->curr_ms < timer->curr_ms)
        {
            timer->curr_ms -= curr->curr_ms;
            pre = curr;
            continue;
        }
        else if (curr->curr_ms == timer->curr_ms)
        {
            timer->curr_ms = 0;
            os_list_insert_after(&os_timer_control.timer_list, curr, timer);
            return;
        }
        else /* curr->curr_ms > timer->curr_ms */
        {
            curr->curr_ms -= timer->curr_ms;
            if (pre != OS_NULL)
            {
                os_list_insert_after(&os_timer_control.timer_list, pre, timer);
            }
            else
            {
                os_list_insert_first(&os_timer_control.timer_list, timer);
            }
            return;
        }
    }
    if (curr == OS_NULL)
    {
        os_list_insert_last(&os_timer_control.timer_list, timer);
    }
}

/* 调试函数：打印当前定时器链表 */
#if OS_TIMER_DBG_PRINTF_EN
static void timer_list_check(void)
{
    int indx = 0;
    os_dbg("---------------timer list  check start-------------");
    os_list_for_each(curr, os_timer_t, &os_timer_control.timer_list)
    {
        os_dbg("[%d]timer name:%s ,period = %s,curr_ms:%d ,reload_ms:%d",
               indx++,
               curr->name,
               curr->state & OS_TIMER_PERIOD ? "yes" : "no",
               curr->curr_ms,
               curr->reload_ms);
    }
    os_dbg("---------------timer list  check end-------------");
}
#else
#define timer_list_check
#endif

/* 定时器初始化内部函数 */
static os_err_t timer_init(os_timer_t *timer, const char *name, int tmo_ms,
                           os_timer_func_t func, void *arg, int flags)
{
    strncpy(timer->name, name, OS_TIMER_NAME_MAX - 1);
    timer->name[OS_TIMER_NAME_MAX - 1] = '\0';
    timer->state = flags;

    timer->reload_ms = tmo_ms;
    timer->curr_ms = timer->reload_ms;

    os_list_item_init(&timer->item, os_timer_t, item);

    timer->func = func;
    timer->arg = arg;
    return OS_ERR_OK;
}

/* 创建静态定时器 */
os_err_t os_timer_create_static(os_timer_t *timer, const char *name, int tmo_ms,
                                os_timer_func_t func, void *arg, int flags)
{
    os_param_failed(timer == OS_NULL, OS_ERR_PARAM);
    os_param_failed(tmo_ms <= 0, OS_ERR_PARAM);
    os_param_failed(func == OS_NULL, OS_ERR_PARAM);

    return timer_init(timer, name, tmo_ms, func, arg, flags);
}

/* 反初始化定时器（目前仅做参数检查） */
os_err_t os_timer_uninit(os_timer_t *timer)
{
    os_param_failed(timer == OS_NULL, OS_ERR_PARAM);

    return OS_ERR_OK;
}

#if OS_MEN_EN
/* 创建动态定时器（从堆上分配） */
os_timer_t *os_timer_create(const char *name, int tmo_ms,
                            os_timer_func_t func, void *arg, int flags)
{
    os_param_failed(tmo_ms <= 0, OS_NULL);
    os_param_failed(func == OS_NULL, OS_NULL);

    os_timer_t *timer = (os_timer_t *)os_mem_malloc(sizeof(os_timer_t));
    if (timer == OS_NULL)
    {
        os_dbg("error:timer init failed");
        return OS_NULL;
    }

    os_err_t err = timer_init(timer, name, tmo_ms, func, arg, flags | OS_FLAG_MEM_HEAP);
    if (err < 0)
    {
        os_dbg("timer init failed");
        os_men_free(timer);
        return OS_NULL;
    }
    return timer;
}
#endif

/* 释放定时器（如果是堆分配的则释放内存） */
os_err_t os_timer_free(os_timer_t *timer)
{
    os_param_failed(timer == OS_NULL, OS_ERR_PARAM);
    os_err_t err = os_timer_uninit(timer);
    if (err < 0)
    {
        os_dbg("error:timer free uninit failed\r\n");
        return err;
    }

    if (timer->state & OS_FLAG_MEM_HEAP)
    {
        os_men_free(timer);
    }
    return OS_ERR_OK;
}

/* 启动定时器 */
os_err_t os_timer_start(os_timer_t *timer)
{
    os_param_failed(timer == OS_NULL, OS_ERR_PARAM);
    timer_list_start_protect();

    if (timer->state & OS_TIMER_START)
    {
        os_dbg("timer: %s is already start", timer->name);
        timer_list_end_protect();
        return os_ERR_STATE;
    }
    timer->curr_ms = timer->reload_ms;
    timer->state |= OS_TIMER_START;
    insert_timer_list(timer);

#if OS_TIMER_DBG_PRINTF_EN
    timer_list_check();
#endif

    os_isr_status_t status =  os_sched_isr_disable();
    os_event_notify(&os_timer_control.timer_list_event);
    os_sched_isr_enable(status);
    os_sched_run();

    timer_list_end_protect();
    return OS_ERR_OK;
}

/* 停止定时器 */
os_err_t os_timer_stop(os_timer_t *timer)
{
    os_param_failed(timer == OS_NULL,OS_ERR_PARAM);
    timer_list_start_protect();
    if(!(timer->state & OS_TIMER_START))
    {
        return os_ERR_STATE;
    }

    /*存在一个特殊情况，用户在回调函数里面调用停止函数，那么在调用函数之前，timer是不在timer_list里面的
      在timeout_list里面，此时不能把它移除timer_list*/
    if((timer->state & OS_TIMER_RUNNING)  == 0)
    {
        os_timer_t * next = os_list_item_next(&os_timer_control.timer_list,timer);
        if(next != OS_NULL)
        {
            next->curr_ms += timer->curr_ms;
        }
        os_list_remove_item(&os_timer_control.timer_list,timer);
    }

    timer->state &= ~ OS_TIMER_START;
    timer_list_check();

    timer_list_end_protect();
    return OS_ERR_OK;
}

#if OS_TIMER_INFO_EN
/* 返回定时器剩余时间 */
int os_timer_left_time(os_timer_t *timer)
{
    os_param_failed(timer == OS_NULL,0);
    timer_list_start_protect();

    int ms = 0;
    if(timer->state & OS_TIMER_START)
    {
            os_list_for_each(curr,os_timer_t,&os_timer_control.timer_list)
        {
            ms +=curr->curr_ms;
            if(curr->curr_ms == timer->curr_ms)
            {
                break;
            }
        }
    }else
    {
        ms = timer->curr_ms;
    }
    timer_list_end_protect();
    return ms;
}

/* 返回定时器是否已启动（当前未实现，返回0） */
int os_timer_get_state_started(os_timer_t *timer)
{
    os_param_failed(timer == OS_NULL,0);
    timer_list_start_protect();
    int started =  timer->state & OS_TIMER_START;
    timer_list_end_protect();
    return started;
}

#if OS_SCHED_STACK_CHECK && OS_STACK_GUARD_EN
static uint8_t timer_sever_task_mem[OS_STACK_GUARD_SIZE + OS_TIMER_TASK_STACK_TASK];
#else
static uint8_t timer_sever_task_mem[OS_TIMER_TASK_STACK_TASK];  
#endif

/* 定时器服务任务入口 */
void timer_sever_task_entry(void *arg)
{
    for (;;)
    {
        os_isr_status_t status =  os_sched_isr_disable();
        os_timer_t *timer = (os_timer_t *) os_list_first(&os_timer_control.timer_list);
        if (timer == OS_NULL)
        {
            /*任务会在这里死等，直到有定时器start而唤醒它*/
            os_event_wait(&os_timer_control.timer_list_event,OS_NULL,0);
            os_sched_isr_enable(status);
            os_sched_run();
            /*---------------------*/
            continue;
        }
        os_sched_isr_enable(status);

        /* 记录本次延时前的 tick 值，用于计算实际经过时间 */
        os_tick_t tick = os_get_tick_count();
        /*这里不能使用任务延时，得考虑到再任务延时过程中，插入一个时间很小的定时器，此时如果定时回来，早已超过那个很短的定时器*/
        status =  os_sched_isr_disable();
        os_event_wait(&os_timer_control.timer_list_event,OS_NULL,timer->curr_ms);
        os_sched_isr_enable(status);
        os_sched_run();
        
        /* tick 变量为实际经过的时间片数量 */
        tick = os_get_tick_count() - tick;
        /* diff_ms 为实际经过的毫秒数 */
        int diff_ms = tick * OS_SYSTICK_MS;
        timer_list_start_protect();
        timer = (os_timer_t *) os_list_first(&os_timer_control.timer_list);
        /* 遍历 timer_list，找出到期定时器并移入 timeout 链表 */
        while (timer != OS_NULL)
        {
            /* 剩余时间大于经过时间，说明尚未到期 */
            if (timer->curr_ms > diff_ms)
            {
                timer->curr_ms -= diff_ms;
                break;
            }
            /* 已到期，移入超时链表 */
            os_timer_t * next = os_list_item_next(&os_timer_control.timer_list, timer);
            os_list_remove_item(&os_timer_control.timer_list, timer);
            os_list_insert_last(&os_timer_control.timer_timeout_list, timer);
            os_dbg("timer %s is timeout,diff_ms:%d ,curr:%d", timer->name, diff_ms, timer->curr_ms);

            timer = next;
        }

        /* 处理所有超时定时器的回调函数 */
        while ((timer = os_list_remove_first(&os_timer_control.timer_timeout_list)) != OS_NULL)
        {
            /* 设置运行标志，防止回调中重入 */
            timer->state |= OS_TIMER_RUNNING;
            timer->func(timer, timer->arg);
            timer->state &= ~OS_TIMER_RUNNING;

            /* 回调之后检查定时器是否仍处于启动状态（用户可能在回调中停止了它） */
            if (timer->state & OS_TIMER_START)
            {
                /* 周期定时器：重新装载并插入链表 */
                if (timer->state & OS_TIMER_PERIOD)
                {
                    timer->curr_ms = timer->reload_ms;
                    insert_timer_list(timer);
                    timer_list_check();
                }
                else /* 单次定时器：清除启动标志 */
                {
                    timer->state &= ~OS_TIMER_START;
                }
            }
        }
        /* 解锁定时器链表 */
        timer_list_end_protect();
    }
}

/* 初始化定时器服务任务 */
os_err_t os_timer_server_task_init(void)
{
    os_timer_control_init();
    os_err_t err = os_task_create_static(&os_core.timer_sever_task,
                                         "timer_sever_task",
                                         timer_sever_task_entry,
                                         OS_NULL,
                                         OS_TIMER_SEVER_TASK_PRIO,
                                         timer_sever_task_mem,
                                         sizeof(timer_sever_task_mem));

    if (err < 0)
    {
        os_dbg("timer_sever_task init error");
        return err;
    }
    os_task_start(&os_core.timer_sever_task);
    return err;
}

#endif
#endif