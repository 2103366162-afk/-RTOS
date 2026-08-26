#ifndef USER_RISCV_OS_PLAT_H_
#define USER_RISCV_OS_PLAT_H_

#include <stdint.h>
#define OS_PLAT_STACK_GROWTH_HTL          1
typedef uint32_t cpu_stack_t;
typedef uint32_t os_isr_status_t;
typedef uint64_t os_tick_t;

typedef enum 
{
    OS_TICK_UPDATE_CLEAR =0,
    OS_TICK_UPDATE_ADD

}os_ticks_update_mode;

typedef struct _os_task_ctx_t 
{
    cpu_stack_t pc;

    cpu_stack_t x1_ra;
    cpu_stack_t x2_sp;
    cpu_stack_t x3_gp;
    cpu_stack_t x4_tp;

    cpu_stack_t x5_t0;
    cpu_stack_t x6_t1;
    cpu_stack_t x7_t2;
    cpu_stack_t x8_s0;
    cpu_stack_t x9_s1;
    cpu_stack_t x10_a0;
    cpu_stack_t x11_a1;
    cpu_stack_t x12_a2;
    cpu_stack_t x13_a3;
    cpu_stack_t x14_a4;
    cpu_stack_t x15_a5;
    cpu_stack_t x16_a6;
    cpu_stack_t x17_a7;
    cpu_stack_t x18_s2;
    cpu_stack_t x19_s3;
    cpu_stack_t x20_s4;
    cpu_stack_t x21_s5;
    cpu_stack_t x22_s6;
    cpu_stack_t x23_s7;
    cpu_stack_t x24_s8;
    cpu_stack_t x25_s9;
    cpu_stack_t x26_s10;
    cpu_stack_t x27_s11;
    cpu_stack_t x28_t3;
    cpu_stack_t x29_t4;
    cpu_stack_t x30_t5;
    cpu_stack_t x31_t6;

    cpu_stack_t mstatus;
}os_task_ctx_t;

struct  _os_task_t;

/**/
void os_task_ctx_init(struct _os_task_t * task,
                      void * stack_top,
                      int stack_size,
                      void (*entry)(void * param),
                      void * param );

void os_plat_init(void);

os_isr_status_t os_plat_isr_disable(void);
void os_plat_isr_enable(os_isr_status_t mscratch);

void os_plat_ticks_update(os_tick_t * ticks,os_ticks_update_mode update_mode);
#endif
