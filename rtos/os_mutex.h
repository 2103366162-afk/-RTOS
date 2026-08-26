#ifndef USER_RTOS_OS_MUTEX_H_
#define USER_RTOS_OS_MUTEX_H_

#include "os_def.h"
#include "os_event.h"

#if OS_MUTEX_EN

typedef struct _os_mutex_t
{
    os_event_t event;
    os_task_t *owner;

    uint16_t locked_cnt;
    uint8_t raw_prio;  /*原始优先级*/
}os_mutex_t;

os_err_t os_mutex_create_static (os_mutex_t * mutex);
os_err_t os_mutex_uninit (os_mutex_t * mutex);       
os_mutex_t * os_mutex_create (void);
os_err_t os_mutex_free (os_mutex_t * mutex);
os_err_t os_mutex_lock (os_mutex_t * mutex, int ms);
os_err_t os_mutex_unlock (os_mutex_t * mutex);

#if OS_MUTEX_INFO_EN
uint16_t os_mutex_lock_cnt (os_mutex_t * mutex);
uint16_t os_mutex_wait_task_cnt (os_mutex_t * mutex);
os_task_t * os_mutex_owner (os_mutex_t * mutex);
#endif
#endif
#endif