#ifndef USER_RTOS_OS_DBG_H_
#define USER_RTOS_OS_DBG_H_

#include "os_def.h"
#include "os_plat.h"

#if OS_DBG_PRINTF_EN
#define os_dbg(fmt,...)        \
        do{                         \
        os_printf("[%s(%d)]:",__FUNCTION__,__LINE__);           \
        os_printf(fmt,##__VA_ARGS__);    \
        os_printf("\r\n");             \
        }while(0)
#else
#define os_dbg(fmt,...) 
#endif

#if OS_DBG_ASSERT_EN
#define  os_assert(expr)       \
         do{                    \
            if(!(expr)){       \
                os_printf("assert falied %s(%d): %s\r\n",__FUNCTION__,__LINE__,#expr);  \
                os_plat_isr_disable();\
                while(1);       \
            }                   \
         }while(0)   
#else
#define os_assert(exper)                       
#endif

#if OS_DBG_PARAM_CHECK_EN
#define os_param_failed(expr,var)                \
        do{                                      \
            if(expr){                            \
                os_dbg("%s\r\n",#expr);          \
                return var;                      \
            }                                    \
        }while(0)

#define os_param_failed_exec(expr,var,exec)      \
        do{                                      \
            if(expr){                            \
                {exec;}                          \
                os_dbg("%s\r\n",#expr);          \
                return var;                      \
            }                                    \
        }while(0)        
#else 
#define os_param_failed(expr,var)
#define os_param_failed_exec(expr,ret,exec)
#endif

#endif