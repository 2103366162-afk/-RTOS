#include "os_sched.h"
#include "os_plat.h"
#include "os_list.h"
#include "os_task.h"
#include "os_idle.h"
#include "os_timer.h"
#include <string.h>

os_core_t os_core;

// 如果开启了位图检查，则将调试器打印强制开启，assert也强制开启，不受os_def.h中的影响
#if OS_SCHED_BITMAP_CHECK_EN || OS_DBG_SCHED_CHECK_EN
#undef OS_DBG_PRINT_ENABLE
#define OS_DBG_PRINT_ENABLE     1       // 强制允许输出
#undef OS_ASSERT_ENABLE
#define OS_ASSERT_ENABLE        1       // 允许assert
#endif

/**
 * 检查位图的正确性
*/
#if OS_SCHED_BITMAP_CHECK_EN
void os_bitmap_check (void) {

    // 不计算idle所在的最低优先级队列
    for (int i = 0; i <= OS_TASK_IDLE_PRIO; i++) {
        // 检查位的设置
        int group = i >> 3;
        int group_map = os_core.ready_map[group];
        if (os_core.ready_group & (1 << group)) {
            os_assert(group_map != 0);
        } else {
            // 没有任务在里面
            os_assert(group_map == 0);
        }

        os_list_t * list = &os_core.ready_list[i];
        if (group_map & (1 << (i & 0x7))) {
            os_assert(os_list_first(list) != OS_NULL);
        } else {
            os_assert(os_list_first(list) == OS_NULL);
        }
    }

    // 最低优先级必须总是置1
    os_assert(os_core.ready_map[(OS_TASK_IDLE_PRIO) >> 3] != 0);
}
#else
#define os_bitmap_check(bitmap)
#endif

os_task_t * os_task_self(void)
{
    return os_core.curr_task;
}

os_isr_status_t os_sched_isr_disable(void)
{
    return os_plat_isr_disable() ;

}

void os_sched_isr_enable(os_isr_status_t mstatus)
{
    return  os_plat_isr_enable(mstatus);
}

void sched_init_ready_list(void)
{
    os_core.curr_task = OS_NULL;
    os_core.next_task =OS_NULL;

    os_core.ready_group = 0;
    for(uint8_t i=0;i < sizeof(os_core.ready_map);i++)
    {
        os_core.ready_map[i] = 0;
    }

    for(uint8_t i = 0; i<=OS_TASK_PIRO_MAX;i++)
    {
        os_list_init(&os_core.ready_list[i],os_task_t,ready_item);
    }
}

#if OS_SCHED_LOCK_EN
void os_sched_lock(void)
{
    os_isr_status_t status =  os_sched_isr_disable();
    if(os_core.sched_lock_count <255)
    {
        os_core.sched_lock_count ++;
    }
    os_sched_isr_enable(status);
}

void os_sched_unlock(void)
{
    os_isr_status_t status =  os_sched_isr_disable();
    if(os_core.sched_lock_count > 0)
    {
        if(--os_core.sched_lock_count == 0)
        {
            os_sched_run();
        }
    }
    os_sched_isr_enable(status);
}

int os_sched_get_lock__count(void)
{
    os_isr_status_t status =  os_sched_isr_disable();
    int cnt = os_core.sched_lock_count;
    os_sched_isr_enable(status);
    return cnt;
}
#endif

void os_sched_init()
{
    sched_init_ready_list();
    os_list_init(&os_core.all_list,os_task_t,info_item);
    os_list_init(&os_core.delay_list,os_task_t,ready_item);
    os_task_idle_init();
#if OS_TIMER_EN
    os_timer_server_task_init();
#endif
#if OS_TASK_EXIT_EN
    os_list_init(&os_core.delete_list,os_task_t,ready_item);
#endif

#if OS_TASK_SUSPEND_EN
    os_list_init(&os_core.suspend_list,os_task_t,ready_item);
#endif
    os_core.os_sched_in_isr_flag = 0;
    os_core.isr_nested = 0;
    os_plat_ticks_update(&os_core.os_tick_count,OS_TICK_UPDATE_CLEAR);
#if OS_SCHED_LOCK_EN
    os_core.sched_lock_count = 0;
#endif
}

void os_sched_run_first(void)
{
    os_core.curr_task =  sched_next_run_task();
    os_assert(os_core.curr_task !=OS_NULL);
    os_dbg("first task is :%s",os_core.curr_task->Name);
    os_task_switch_to (&os_core.curr_task->ctx);
}
void os_shced_switch_ctx(void)
{
   os_core.curr_task = os_core.next_task;
}

void os_sched_add_new(os_task_t * task)
{
    os_assert(task != OS_NULL);
    os_list_insert_last(&os_core.all_list,task);
}

void os_sched_remove_all_list(os_task_t * task)
{
    os_assert(task != OS_NULL);
    os_list_remove_item(&os_core.all_list,task);
}

void os_sched_set_ready(os_task_t * task)
{
    os_assert(task != OS_NULL);
    os_assert((task->prio >= 0 ) && ( task->prio <= OS_TASK_PIRO_MAX));
    os_assert(!(task->task_flags & OS_TASK_READY_FLAG));   /* 不能已经在就绪态 */
    os_list_insert_last(&os_core.ready_list[task->prio],task);
    // 一个数字右移三位>>3，相当于除以八
    os_core.ready_map[task->prio >> 3 ] |= (1 << ((task->prio) % 8));
    os_core.ready_group |= 1 << ((task->prio >> 3));

    task->task_flags |= OS_TASK_READY_FLAG; 
#if OS_SCHED_RR_EN
    task->slice = OS_TASK_SLICE;
#endif
   os_bitmap_check();
}

void os_sched_set_delay(os_task_t * task,int ms)
{
    os_assert(task != OS_NULL);
    task->task_flags &= ~ OS_TASK_READY_FLAG;

    int tick = (ms + OS_SYSTICK_MS - 1) / OS_SYSTICK_MS;
    if(tick <= 0)
    {
        tick = 1;
    }
    task->delay_tick = tick;
    task->task_flags |= OS_TASK_DELAY_FLAG;
#if OS_SCHED_DELAY_INSERT_LAST
    os_list_insert_last(&os_core.delay_list,task);
#else
    os_list_for_each(curr,os_task_t,&os_core.delay_list)
    {
        if(curr->delay_tick < task->delay_tick)
        {
            task->delay_tick -= curr->delay_tick;
            continue;
        }else if(curr->delay_tick == task->delay_tick)
        {
            task->delay_tick = 0;
            os_list_insert_after(&os_core.delay_list,curr,task);
            return;
        }else 
        {
            curr->delay_tick -= task ->delay_tick;
            os_task_t * pre_task = os_list_item_pre(&os_core.delay_list,curr);
            os_list_insert_after(&os_core.delay_list,pre_task,task);
            return;
        }
    }
    os_list_insert_last(&os_core.delay_list,task);
#endif
}

void os_sched_remove_ready(os_task_t * task)
{
    os_assert(task != OS_NULL);
    os_assert(task->task_flags & OS_TASK_READY_FLAG);   /* 必须已经在就绪态 */

    os_list_remove_item(&os_core.ready_list[task->prio], task);

    if(os_core.ready_list[task->prio].count == 0)
    {
        os_core.ready_map[task->prio >> 3] &= ~(1 << (task->prio % 8));
        if(os_core.ready_map[task->prio >> 3] == 0)
        {
            os_core.ready_group &= ~(1 << (task->prio >> 3));
        }
    }

    task->task_flags &= ~OS_TASK_READY_FLAG;
}

void os_sched_remove_delay(os_task_t * task)
{
#if OS_SCHED_DELAY_INSERT_LAST
    os_assert(task != OS_NULL);
    os_list_remove_item (&os_core.delay_list, task);
    task->task_flags &= ~ OS_TASK_DELAY_FLAG;
#else
    os_assert(task != OS_NULL);
    os_task_t * next = os_list_item_next(&os_core.delay_list,task);
    if(next != OS_NULL)
    {
        next->delay_tick += task->delay_tick;
    }
    os_list_remove_item (&os_core.delay_list, task);
    task->task_flags &= ~ OS_TASK_DELAY_FLAG;
#endif
}

os_task_t * sched_next_run_task(void)
{
    // 优先级映射表：输入 0~255，返回最低位 1 的索引 (0~7)
    const uint8_t map_table[256] = {
    0xff, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,  // 0x00~0x0F
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,  // 0x10~0x1F
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,  // 0x20~0x2F
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,  // 0x30~0x3F
    6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,  // 0x40~0x4F
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,  // 0x50~0x5F
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,  // 0x60~0x6F
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,  // 0x70~0x7F
    7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,  // 0x80~0x8F
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,  // 0x90~0x9F
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,  // 0xA0~0xAF
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,  // 0xB0~0xBF
    6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,  // 0xC0~0xCF
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,  // 0xD0~0xDF
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,  // 0xE0~0xEF
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0   // 0xF0~0xFF
    };

    /*当就绪列表中没有任务时，os_core.ready_group =0，查表得知是0xff，此时会卡在os_assert(g >=0 && g<=7);*/
    uint8_t g = map_table[os_core.ready_group];
    os_assert(g >=0 && g<=7);    
    uint8_t b = map_table[os_core.ready_map[g]];
    os_assert(b >=0 && b<=7);
    uint8_t prio = (g << 3) + b;
    os_assert(prio>=0 && prio<=OS_TASK_PIRO_MAX);

    os_task_t * next = os_list_first(&os_core.ready_list[prio]);  

    return (next != os_core.curr_task) ?  next : OS_NULL;
}

#if OS_TASK_EXIT_EN
void os_sched_set_delete(os_task_t * task)
{
    os_assert(task != OS_NULL);
    task->task_flags |= OS_TASK_DELETE_FLAG;
    os_list_insert_last(&os_core.delete_list,task);  
}

os_task_t * os_sched_remove_delete(void)
{
    if(os_core.delete_list.count == 0)
    {
        return OS_NULL;
    }
    os_isr_status_t status = os_sched_isr_disable();

    os_task_t * delete_task = os_list_remove_first(&os_core.delete_list);

    os_sched_isr_enable(status);

    return delete_task; 
}
#endif

#if OS_TASK_SUSPEND_EN
void os_sched_set_suspend(os_task_t * task)
{
    os_assert(task != OS_NULL);
    os_list_insert_last(&os_core.suspend_list,task);
    task->task_flags |= OS_TASK_SUSPEND_FLAG;
}

void os_sched_remove_suspend(os_task_t * task)
{
    os_assert(task != OS_NULL);
    os_list_remove_item(&os_core.suspend_list,task);
    task->task_flags &= ~OS_TASK_SUSPEND_FLAG;
}
#endif

#if OS_SCHED_STACK_CHECK 

#if OS_STACK_GUARD_EN
/* 检查 guard 完整性：被破坏则返回 1，正常返回 0 */
static os_err_t os_guard_check(const os_task_t * task)
{
    const uint8_t * g = (const uint8_t *)task->guard_start;
    for (int i = 0; i < task->guard_size; i++) {
        if (g[i] != OS_STACK_GUARD_FILL) {
            return OS_ERR_CHECK;   /* 被改写 */
        }
    }
    return OS_ERR_OK;
}
#else
#define os_guard_check()    
#endif

os_err_t os_sched_stack_check(os_task_t * task)
{
    uint16_t free_bytes = 0;
#if OS_PLAT_STACK_GROWTH_HTL
    uint8_t * start_stack = (uint8_t *) task->start_stack;  //start_stack是任务栈的起始地址，但是是最后存数据的地方，也就是是栈的末尾
     for(int i=0;i<task->stack_size;i++)
     {
        if(*start_stack++ != OS_TASK_STACK_FILL)
        {
            break;
        }
        free_bytes++;
     }
#else
    //这里是sp往高地址生长的情况，这里还未实现
#endif
    //OS_SCHED_STACK_THREHOLD为设定的报警阈值
    if(free_bytes< OS_SCHED_STACK_THREHOLD)
    {
        os_dbg("task %s stack check failed : unused = %d < threshold = %d\r\n",
                task->Name , free_bytes, OS_SCHED_STACK_THREHOLD);

    /* 3. guard 完整性检查（历史越界证据） */
#if OS_STACK_GUARD_EN
        if (os_guard_check(task) == OS_ERR_CHECK) 
        {
            os_dbg("[STACK] %s REDZONE 被破坏", task->Name);
            os_assert(0);
            return OS_ERR_STACK;
        }
#endif
        return OS_ERR_STACK; 
    }
    return OS_ERR_OK;
}
#endif

void os_sched_run(void)
{
    os_isr_status_t status = os_sched_isr_disable();

#if OS_SCHED_LOCK_EN
    if(os_core.sched_lock_count >0)
    {
        os_sched_isr_enable(status);
        return;
    }
#endif

    if(os_core.isr_nested >0)
    {
        os_sched_isr_enable(status);
        return;
    }

    if(os_core.curr_task == OS_NULL)
    {
        os_sched_isr_enable(status);
        return;
    }

    os_task_t * next; 
#if OS_SCHED_STACK_CHECK 
resched:
#endif
    next = sched_next_run_task();

    if(next == OS_NULL)
    {
        os_core.os_sched_in_isr_flag = 0;
        os_sched_isr_enable(status);
        return;
    }

#if OS_SCHED_STACK_CHECK 
    os_err_t err = os_sched_stack_check(next);
    if(err < 0)
    {
        if(next->prio == OS_TASK_IDLE_PRIO)
        {
            os_dbg("%s:stack check failed\r\n",next->Name);
            os_assert(0);
        }else
        {
            os_dbg("%s:stack check failed\r\n",next->Name);
            os_sched_remove_ready(next);
            goto resched;
        }
    }

#endif

    os_core.next_task = next;
    if(os_core.os_sched_in_isr_flag)
    {
        os_core.os_sched_in_isr_flag = 0;
#if OS_DBG_SCHED_CHECK_EN
        os_dbg("isr switch from %s to %s",os_core.curr_task->Name,os_core.next_task->Name);
#endif

        os_task_switch_from_isr(&os_core.curr_task->ctx,&os_core.next_task->ctx);
    }else
    {
#if OS_DBG_SCHED_CHECK_EN
        os_dbg("task switch from %s to %s",os_core.curr_task->Name,os_core.next_task->Name);
#endif
        os_task_switch(&os_core.curr_task->ctx,&os_core.next_task->ctx);
    }
    os_sched_isr_enable(status);
}

void os_sched_yield(void)
{
    os_task_t * self = os_task_self();

    os_isr_status_t status = os_sched_isr_disable();

    os_list_t * list = &os_core.ready_list[self->prio];

    os_task_t * next = os_list_item_next(list,self);
    if(next != OS_NULL)
    {
        os_sched_remove_ready(self);
        os_sched_set_ready(self);

        os_sched_run();
        // os_core.curr_task = next;

        // os_task_switch(&self->ctx,&next->ctx);
    }
    os_sched_isr_enable(status);
}

void os_sched_time_tick(void)
{
    
    os_isr_status_t status = os_sched_isr_disable();

#if OS_SCHED_DELAY_INSERT_LAST
    os_task_t * task = (os_task_t *)os_list_first(&os_core.delay_list);
    while(task)
    {   
        os_task_t * next = (os_task_t *) os_list_item_next(&os_core.delay_list,task);
        if(--task->delay_tick==0)
        {
            os_sched_remove_delay(task);
            os_sched_set_ready(task);    //此时task的next是0，必须先获得next再remove和set
        }
        task = next;
    }
#else
    os_task_t * task = (os_task_t *)os_list_first(&os_core.delay_list);
    if((task  != OS_NULL) && (-- task->delay_tick == 0) )
    {
        do
        {
            os_task_t * next = (os_task_t *) os_list_item_next(&os_core.delay_list,task);
            if(task->task_flags & OS_TASK_EVENT_WAIT)
            {
                os_event_notify_task(task->event_info.event,task,OS_NULL,OS_ERR_EVENT_TIMEOUT);
            }else
            {
                os_sched_remove_delay(task);
                os_sched_set_ready(task);
            }
            task = next;
        }while( (task != OS_NULL) && (task->delay_tick == 0) );
    }
#endif

#if OS_SCHED_RR_EN
    if(os_core.curr_task)
    {
        if(--os_core.curr_task->slice == 0)
        {

            os_core.curr_task->slice = OS_TASK_SLICE; 
            if(os_list_item_next(&os_core.ready_list[os_core.curr_task->prio],os_core.curr_task))
            {   
                os_sched_remove_ready(os_core.curr_task);
                os_sched_set_ready(os_core.curr_task);
            }
            
        }
    }
#endif
    os_plat_ticks_update(&os_core.os_tick_count,OS_TICK_UPDATE_ADD);
    os_sched_isr_enable(status);
}

os_tick_t os_get_tick_count(void)
{
    os_isr_status_t status = os_sched_isr_disable();
    os_tick_t tick = os_core.os_tick_count;
    os_sched_isr_enable(status);
    return tick;
}

void os_isr_enter(void)
{
    os_isr_status_t status =  os_sched_isr_disable();
    os_core.isr_nested += 1;
    os_sched_isr_enable(status);
}

void os_isr_leave(void)
{
    os_isr_status_t status =  os_sched_isr_disable();
    if(-- os_core.isr_nested == 0)
    {
        // os_sched_isr_enable(status);
        os_core.os_sched_in_isr_flag =1;
        os_sched_run();
    }
    os_sched_isr_enable(status);
}

