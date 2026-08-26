#ifndef USER_RTOS_OS_ERR_H_
#define USER_RTOS_OS_ERR_H_

#define OS_NULL          ((void *)0)

typedef enum _os_err_t
{
    OS_ERR_OK = 0,
    OS_ERR_NAME = -1,
    os_ERR_STATE = -2,
    OS_ERR_PARAM = -3,
    OS_ERR_STACK = -4,
    OS_ERR_CHECK = -5,
    OS_ERR_CREATE = -6,
    OS_ERR_EVENT_TIMEOUT = -7,
    OS_ERR_REMOVE   =-8,
    OS_ERR_NONE = -9,
    OS_ERR_LOCKED = -10,
    OS_ERR_UNLOCKED = -11,
    OS_ERR_OWNER = -12,
    OS_ERR_ISR = -13,

    /*计数型信号量*/
    OS_ERR_SEM_CNT_ZERO = -14,
    OS_ERR_SEM_CNT_OUT = -15,

    /*消息队列*/
    OS_ERR_QUEUE_POOL_FULL = -16,
    OS_ERR_QUEUE_POOL_EMPTY = -17,
    OS_ERR_QUEUE_CLAER = -18,

    /*时间标志组*/
    OS_ERR_EFLAGS_FLAG_NOT_SATISFY = -19,
}os_err_t;



#endif