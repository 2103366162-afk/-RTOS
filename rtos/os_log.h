#ifndef USER_RTOS_OS_LOG_H_
#define USER_RTOS_OS_LOG_H_

#include "os_lib.h"
#include "os_def.h"

#if OS_LOG_ERR_EN
#define os_log_err(fmt,...)        \
        do{                         \
        os_printf("[err %s %d]:",__FUNCTION__,__LINE__);           \
        os_printf(fmt,##__VA_ARGS__);    \
        os_printf("\r\n");             \
        }while(0)
#else
#define os_log_err(fmt,...) 
#endif

#if OS_LOG_INFO_EN
#define os_log_info(fmt,...)        \
        do{                         \
        os_printf("[info %s %d]:",__FUNCTION__,__LINE__);           \
        os_printf(fmt,##__VA_ARGS__);    \
        os_printf("\r\n");             \
        }while(0)
    
#else
#define os_log_info(fmt,...)
#endif

#endif