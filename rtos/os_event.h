#ifndef USER_RTOS_OS_EVENT_H_
#define USER_RTOS_OS_EVENT_H_
#include "os_list.h"
#include "os_def.h"

typedef enum _os_event_type_t
{
    OS_EVENT_TYPE_INVALID,

    OS_EVENT_TYPE_SEM,
    OS_EVENT_TYPE_QUEUE,
    OS_EVENT_TYPE_MBLOCK,
    OS_EVENT_TYPE_EFLAGS,
    OS_EVENT_TYPE_MUTEX,
    OS_EVENT_TYPE_TIMER,

    OS_EVENT_TYPE_MAX,
}os_event_type_t;

typedef struct _os_event_t 
{
    os_event_type_t type;
    int flag;
    os_list_t wait_list;
}os_event_t;


os_err_t os_event_init (os_event_t * event, os_event_type_t type, int flags);
os_err_t os_event_uninit (os_event_t * event);
void os_event_wait (os_event_t * event, void * reason, int ms);
struct _os_task_t * os_event_notify (os_event_t * event);
void os_event_insert_task(os_event_t *event, os_task_t *task);
void os_event_notify_task (os_event_t * event, struct _os_task_t * task, void * reason, os_err_t err);
void os_event_remove_task(os_event_t *event,os_task_t *task);
void os_event_notify_all (os_event_t * event, void * reason, os_err_t err);
int os_event_wait_cnt (os_event_t * event);

#endif

