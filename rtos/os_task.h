#ifndef USER_RTOS_OS_TASK_H_
#define USER_RTOS_OS_TASK_H_

#include <stdint.h>
#include "os_def.h"
#include "os_plat.h"
#include "os_err.h"
#include "os_list.h"
#include "os_event.h"

#define os_plat_use_isr_sp_in()  asm("csrrw sp,mscratch,sp") // mscratch和sp的值互换，分两步，第一步：把mscratch赋值给sp，把旧sp的值赋值给mscratch
#define os_plat_use_isr_sp_out()  asm("csrrw sp,mscratch,sp")

// #define os_isr_enable()     __set_MSTATUS(0x1888)
// #define os_isr_disenable()  __set_MSTATUS(0x1880)

#define OS_TASK_CREATE_MEM_FLAG (1<<0)
#define OS_TASK_READY_FLAG    (1<<8)
#define OS_TASK_DELAY_FLAG    (1<<9)
#define OS_TASK_DELETE_FLAG    (1<<10)
#define OS_TASK_SUSPEND_FLAG    (1<<11)
#define OS_TASK_EVENT_WAIT      (1<<12)

typedef  uint32_t os_isr_status_t;

os_isr_status_t os__isr_disable(void);
void os_sched_isr_enable(os_isr_status_t mscratch);

typedef void (*taks_entry_t)(void * param);

typedef  struct _task_wait_t
{
    os_event_t * event;
    os_err_t err;
    void * reason;
}task_wait_t;

typedef struct _os_task_t
{
    char Name[OS_TASK_NAME_SIZE];
    os_task_ctx_t ctx;

    uint16_t task_flags;
    uint16_t prio;

    /* 任务栈信息：无论是否启用栈检测，都始终记录，用于内存释放及调试 */
    void * start_stack;      /* 实际栈区起始地址（若启用 guard，则为 guard 之后的有效栈起始） */
    int    stack_size;       /* 实际栈区大小（已扣除 guard 大小） */

#if OS_STACK_GUARD_EN       /* 如果启用了栈保护区域（依赖 OS_SCHED_STACK_CHECK） */
    void * guard_start;      /* guard 区域起始地址 */
    int    guard_size;       /* guard 区域大小 */
#endif

#if OS_SCHED_RR_EN
    int slice;
#endif

    int delay_tick;

    os_list_item_t  info_item;
    os_list_item_t  ready_item;
    os_list_item_t  event_item;

    task_wait_t event_info;
}os_task_t;

void task_event_wait_init(task_wait_t * event_info,os_event_t * event,void *reason);

os_task_t * os_task_create(const char *name,
                           taks_entry_t task_entry,
                           void *param,
                           int priority,
                           int stack_size);

os_err_t os_task_create_static(os_task_t * task,
                      const char *name,
                      taks_entry_t task_entry,
                      void *param,
                      int priority,
                      void *stack_top,
                      int stack_size);

void os_task_start(os_task_t * task);
void os_task_delay(int ms);

#if OS_TASK_EXIT_EN
void os_task_exit();
#endif

#if OS_TASK_SUSPEND_EN
os_err_t os_task_suspend(os_task_t * task);
os_err_t os_task_resume(os_task_t * task);
#endif

#endif