#ifndef USER_RTOS_OS_TIMER_H_
#define USER_RTOS_OS_TIMER_H_
#include "os_def.h"
#include "os_list.h"
#if OS_TIMER_EN

#define OS_TIMER_ONE_SHOT       (0<<10)     /*定时器单次模式*/   
#define OS_TIMER_PERIOD         (1<<10)     /*定时器循环模式*/
#define OS_TIMER_RUNNING        (1<<9)      /*定时器函数正在运行*/
#define OS_TIMER_START          (1<<8)      /*定时器已经启动*/
struct _os_timer_t;
typedef void (*os_timer_func_t) (struct _os_timer_t * timer, void * arg);

typedef  struct _os_timer_t
{
    char name[OS_TIMER_NAME_MAX];
    int state;

    uint16_t curr_ms;
    uint16_t reload_ms;

    os_list_item_t item;

    os_timer_func_t func;
    void * arg;
}os_timer_t;

os_err_t os_timer_create_static (os_timer_t * timer, const char * name, int tmo_ms, os_timer_func_t func, void * arg, int flags);
os_err_t os_timer_uninit(os_timer_t * timer);
#if OS_MEN_EN
os_timer_t * os_timer_create (const char * name, int tmo_ms, os_timer_func_t func, void * arg, int flags);
#endif
os_err_t os_timer_free (os_timer_t * timer);
os_err_t os_timer_start (os_timer_t * timer);
os_err_t os_timer_stop (os_timer_t * timer);
int os_timer_left (os_timer_t * timer);
int os_timer_get_state_started (os_timer_t * timer);

os_err_t os_timer_server_task_init(void);

#endif
#endif