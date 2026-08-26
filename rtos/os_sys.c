#include "os_sys.h"

os_err_t os_sys_init(void)
{
    USART_Printf_Init(115200);
    os_plat_init();
    os_sched_init();
#if OS_MEN_EN
    os_men_init();
#endif
    os_dbg("rtos init");
    os_dbg("RTOS VERSION:%s",OS_VERSION);
    return OS_ERR_OK ;
}

void os_sys_start(void)
{
    os_sched_run_first();

    // 不应该运行到这里
    os_dbg("os_start error, should not get here...\n");
    for (;;) {}
}