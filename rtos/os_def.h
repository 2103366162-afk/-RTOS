#ifndef USER_RTOS_OS_DEF_H_
#define USER_RTOS_OS_DEF_H_

#include "os_conf.h"

#define OS_VERSION                         "1.0"

/* 任务名长度 */
#ifndef OS_TASK_NAME_SIZE
#define OS_TASK_NAME_SIZE                  20
#endif

/* 日志与断言开关 */
#ifndef OS_LOG_ERR_EN
#define OS_LOG_ERR_EN                      1
#endif
#ifndef OS_LOG_INFO_EN
#define OS_LOG_INFO_EN                     1
#endif
#ifndef OS_DBG_PRINTF_EN
#define OS_DBG_PRINTF_EN                   1
#endif
#ifndef OS_DBG_ASSERT_EN
#define OS_DBG_ASSERT_EN                   1
#endif
#ifndef OS_DBG_PARAM_CHECK_EN
#define OS_DBG_PARAM_CHECK_EN              1
#endif
#ifndef OS_DBG_SCHED_CHECK_EN
#define OS_DBG_SCHED_CHECK_EN              0
#endif

/* 栈检测与守护区配置 */
#ifndef OS_SCHED_STACK_CHECK
#define OS_SCHED_STACK_CHECK               1
#define OS_TASK_STACK_FILL                 0xAA
#define OS_SCHED_STACK_THREHOLD            128
#endif


#ifndef OS_STACK_GUARD_EN
#define OS_STACK_GUARD_EN                  1    /* 1: 启用，0: 关闭 */
#endif
#if OS_STACK_GUARD_EN
#ifndef OS_STACK_GUARD_SIZE
#define OS_STACK_GUARD_SIZE                32   /* 字节，必须是4的倍数 */
#endif
#ifndef OS_STACK_GUARD_FILL
#define OS_STACK_GUARD_FILL                0x5A
#endif
#endif
#endif

/* 列表调试 */
#ifndef OS_LIST_INFO_EN
#define OS_LIST_INFO_EN                    1
#endif

/* Tick 配置 */
#ifndef OS_SYSTICK_MS
#define OS_SYSTICK_MS                      10
#endif

/* 时间片轮转调度 */
#ifndef OS_SCHED_RR_EN
#define OS_SCHED_RR_EN                     1
#define OS_TASK_SLICE                      10
#endif
#ifndef OS_SCHED_DELAY_INSERT_LAST
#define OS_SCHED_DELAY_INSERT_LAST         0
#endif

/* 位图调度检查 */
#ifndef OS_SCHED_BITMAP_CHECK_EN
#define OS_SCHED_BITMAP_CHECK_EN           0
#endif

/* 调度锁 */
#ifndef OS_SCHED_LOCK_EN
#define OS_SCHED_LOCK_EN                   1
#endif

/* 任务优先级 */
#ifndef OS_TASK_PIRO_MAX
#define OS_TASK_PIRO_MAX                   32
#endif

/* IDLE 任务配置 */
#define OS_TASK_IDLE_STACK_TASK            1000
#define OS_TASK_IDLE_PRIO                  OS_TASK_PIRO_MAX

#ifndef OS_TASK_EXIT_EN
#define OS_TASK_EXIT_EN                    1
#endif
#ifndef OS_TASK_SUSPEND_EN
#define OS_TASK_SUSPEND_EN                 1
#endif

/*动态分配内存*/
#ifndef OS_MEN_EN 
#define OS_MEN_EN                          1
#ifndef OS_MEN_INFO_EN                     
#define OS_MEN_INFO_EN                     1   
#ifndef OS_MEN_ALLOC_SIZE                     
#define OS_MEN_ALLOC_SIZE                  (10 *1024)
#endif
#endif
#endif
#if OS_MEN_EN                  
#define OS_MEN_MIN_SIZE                    4                     
#define OS_MEN_ITEM_SIZE                   (MEN_HIGH_ALIGED(sizeof(os_men_item_t),MEN_ALLGN_BYTES))   
#define OS_MEN_DBG_PRINTF_EN               0
#endif
/*计算当前是32位的 还是64位的cpu*/
#ifndef MEN_ALLGN_BYTES
#define MEN_ALLGN_BYTES                    sizeof(void *)
#endif

/*开启定长存储块使能*/
#ifndef OS_MBLOCK_EN                    
#define OS_MBLOCK_EN                       1
#define OS_MBLOCK_INFO_EN                  1
#define OS_MBLOCK_DBG_PRINT_EN             1
#endif

/*互斥锁使能*/
#ifndef OS_MUTEX_EN
#define OS_MUTEX_EN                        1
#define OS_MUTEX_INFO_EN                   1
#define OS_MUTEX_DBG_PRINT_EN              0
#endif


/*计数型信号量使能*/
#ifndef OS_SEM_EN
#define OS_SEM_EN                        1
#define OS_SEM_INFO_EN                   1
#define OS_SEM_DBG_PRINT_EN              1
#endif

/*消息队列使能*/
#ifndef OS_QUEUE_EN
#define OS_QUEUE_EN                        1
#define OS_QUEUE_INFO_EN                   1
#define OS_QUEUE_DBG_PRINT_EN              1
#define OS_QUEUE_URGENT_EN                 1
#endif

/*事件标志组*/
#ifndef OS_EFLAGS_EN
#define OS_EFLAGS_EN                        1
#define OS_EFLAGS_INFO_EN                   1
#endif

/*软件定时器*/
#ifndef OS_TIMER_EN
#define OS_TIMER_EN                        1
#define OS_TIMER_INFO_EN                   1
#define OS_TIMER_DBG_PRINTF_EN             1
#endif
/*定时器任务*/
#if OS_TIMER_EN
#define OS_TIMER_NAME_MAX                  10
#define OS_TIMER_SEVER_TASK_PRIO           0
#define OS_TIMER_TASK_STACK_TASK           1024
#endif       
