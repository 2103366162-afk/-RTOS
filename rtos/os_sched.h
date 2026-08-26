#ifndef USER_RTOS_OS_SCHED_H_
#define USER_RTOS_OS_SCHED_H_

#include <stdint.h>
#include "os_list.h"
#include "os_task.h"
#include "os_men.h"

typedef uint32_t os_isr_status_t;
typedef uint64_t os_tick_t ;

typedef struct _os_core_t 
{
    os_task_t * curr_task;
    os_task_t * next_task;

    uint8_t os_sched_in_isr_flag;

    os_list_t  all_list;
    uint8_t ready_group;
    uint8_t ready_map[8];
    os_list_t  ready_list[OS_TASK_PIRO_MAX + 1];
    os_list_t  delay_list;
#if OS_TASK_EXIT_EN
    os_list_t delete_list;
#endif
#if OS_TASK_SUSPEND_EN
    os_list_t suspend_list;
#endif
#if OS_MEN_EN
    os_men_t heap_men;
#endif
    os_tick_t os_tick_count;

    os_task_t  idle_task;
#if OS_TIMER_EN
    os_task_t timer_sever_task;
#endif

    uint8_t isr_nested;
#if OS_SCHED_LOCK_EN
    int sched_lock_count;
#endif
}os_core_t;

extern os_core_t os_core;

os_isr_status_t os_sched_isr_disable(void);
void os_sched_isr_enable(os_isr_status_t mscratch);
void os_sched_lock(void);
void os_sched_unlock(void);
int os_sched_get_lock__count(void);

void os_sched_init(void);

os_task_t * os_task_self(void);
void os_sched_add_new(os_task_t * task);
void os_sched_remove_all_list(os_task_t * task);
void os_sched_set_ready(os_task_t * task);
void os_sched_remove_ready(os_task_t * task);
void os_sched_set_delay(os_task_t * task,int ms);
void os_sched_remove_delay(os_task_t * task);
#if OS_TASK_EXIT_EN
void os_sched_set_delete(os_task_t * task);
os_task_t * os_sched_remove_delete(void);
#endif
#if OS_TASK_SUSPEND_EN
void os_sched_set_suspend(os_task_t * task);
void os_sched_remove_suspend(os_task_t * task);
#endif

os_task_t * sched_next_run_task(void);
void os_sched_run_first(void);
void os_sched_run(void);
void os_sched_yield(void);
void os_sched_time_tick(void);

void os_task_switch_to (os_task_ctx_t * to);
void os_task_switch (os_task_ctx_t * from, os_task_ctx_t * to);
void os_task_switch_from_isr(os_task_ctx_t * from, os_task_ctx_t * to);
void os_shced_switch_ctx(void);

os_tick_t os_get_tick_count(void);

void os_isr_enter(void);
void os_isr_leave(void);

#if OS_SCHED_BITMAP_CHECK_EN
void os_bitmap_check (void);
#endif

#if OS_SCHED_STACK_CHECK
os_err_t os_sched_stack_check(os_task_t * task);
#endif
#endif