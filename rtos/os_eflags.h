#ifndef USER_RTOS_OS_EFLAGS_H_
#define USER_RTOS_OS_EFLAGS_H_

#include "os_err.h"
#include "os_event.h"

#if OS_EFLAGS_EN


#define	OS_EFLAGS_CLEAR		        (0x0 << 0)
#define	OS_EFLAGS_SET			    (0x1 << 0)
#define	OS_EFLAGS_ANY			    (0x0 << 1)
#define	OS_EFLAGS_ALL			    (0x1 << 1)
#define	OS_EFLAGS_EXIT_CLEAR        (0x1 << 7)      // 退出时清除相关标志



#define OS_EFLAGS_SET_ALL		    (OS_EFLAGS_SET | OS_EFLAGS_ALL)    /*等待指定的位都置1*/
#define	OS_EFLAGS_SET_ANY		    (OS_EFLAGS_SET | OS_EFLAGS_ANY)    /*等待指定的位任意1位置1*/
#define OS_EFLAGS_CLEAR_ALL	        (OS_EFLAGS_CLEAR | OS_EFLAGS_ALL)  /*等待指定的位都置0*/
#define OS_EFLAGS_CLEAR_ANY	        (OS_EFLAGS_CLEAR | OS_EFLAGS_ANY)  /*等待指定的位任意1位置0*/

typedef uint32_t os_flags_t; 

typedef struct _os_eflags_t{
    os_event_t event;
    os_flags_t flags;
}os_eflags_t;

os_err_t os_eflags_create_static (os_eflags_t * eflags, os_flags_t init_flags);
os_err_t os_eflags_uninit(os_eflags_t * eflags);
os_eflags_t * os_eflags_create (os_flags_t init_flags);
os_err_t os_eflags_free (os_eflags_t * eflags);
os_flags_t os_eflags_wait_bits (os_eflags_t * eflags, int ms, int type, os_flags_t flags,  os_err_t * p_err);
os_err_t os_eflags_set_bits (os_eflags_t * eflags, int type, os_flags_t flags);
#if OS_EFLAGS_INFO_EN
os_flags_t os_eflags_get_flags (os_eflags_t * eflags, os_err_t * err);
int os_eflags_wait_task_cnt (os_eflags_t * eflags);

#endif
#endif
#endif
