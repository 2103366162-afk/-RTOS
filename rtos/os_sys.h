#ifndef USER_RISCV_OS_SYS_H_
#define USER_RISCV_OS_SYS_H_

#include "os_def.h"
#include "os_test.h"
#include "os_lib.h"
#include "os_dbg.h"
#include "os_sched.h"
#include "os_task.h"
#include "debug.h"
#include "os_idle.h"
#include "os_men.h"
#include "os_mblock.h"
#include "os_mutex.h"
#include "os_sem.h"
#include "os_queue.h"
#include "os_eflags.h"
#include "os_timer.h"

os_err_t os_sys_init(void);

void os_sys_start(void);

#endif