#ifndef USER_RTOS_OS_SEM_H_
#define USER_RTOS_OS_SEM_H_

#include "os_event.h"

#if OS_SEM_EN

typedef struct _os_sem_t
{       
    os_event_t event;
    uint8_t curr_cnt;    /*¼ÆÊýÖµ·¶Î§0-255*/
    uint8_t max_cnt;       
}os_sem_t;

os_err_t os_sem_create_static (os_sem_t * sem, int init_cnt, int max_cnt);
os_err_t os_sem_uninit(os_sem_t * sem);
os_sem_t * os_sem_create (int init_cnt, int max_cnt);
os_err_t os_sem_free (os_sem_t * sem);
os_err_t os_sem_take (os_sem_t * sem, int ms);
os_err_t os_sem_release (os_sem_t * sem);
os_err_t os_sem_take_from_isr(os_sem_t * sem);
os_err_t os_sem_release_from_isr(os_sem_t * sem);
#if OS_SEM_INFO_EN
int os_sem_cnt (os_sem_t * sem);
int os_sem_max (os_sem_t * sem);
int os_sem_wait_tasks_cnt (os_sem_t * sem);
#endif
#endif
#endif
