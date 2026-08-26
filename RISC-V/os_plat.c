#include "os_dbg.h"
#include "os_plat.h"
#include "os_task.h"
#include "ch32v20x.h"
#include "os_sched.h"

extern uint8_t* __global_pointer$;
extern os_core_t os_core;

void SysTick_Handler(void)__attribute__((interrupt()));


void os_task_ctx_init(struct _os_task_t * task,
                      void * stack_start,
                      int stack_size,
                      void (*entry)(void * param),
                      void * param )
{
    os_task_ctx_t *ctx = &task->ctx;

    ctx->pc    = (cpu_stack_t)entry;
#if OS_TASK_EXIT_EN
    ctx->x1_ra  = (cpu_stack_t)os_task_exit;
#else 
    ctx->x1_ra  = 1;
#endif
    ctx->x2_sp  = (cpu_stack_t)( (uintptr_t)stack_start + stack_size ) & ~0x03UL;
    ctx->x3_gp  = (cpu_stack_t)&__global_pointer$;
    ctx->x4_tp  = 4;

    ctx->x5_t0  = 5;
    ctx->x6_t1  = 6;
    ctx->x7_t2  = 7;
    ctx->x8_s0  = 8;
    ctx->x9_s1  = 9;
    ctx->x10_a0 = (cpu_stack_t)param;
    ctx->x11_a1 = 11;
    ctx->x12_a2 = 12;
    ctx->x13_a3 = 13;
    ctx->x14_a4 = 14;
    ctx->x15_a5 = 15;
    ctx->x16_a6 = 16;
    ctx->x17_a7 = 17;
    ctx->x18_s2 = 18;
    ctx->x19_s3 = 19;
    ctx->x20_s4 = 20;
    ctx->x21_s5 = 21;
    ctx->x22_s6 = 22;
    ctx->x23_s7 = 23;
    ctx->x24_s8 = 24;
    ctx->x25_s9 = 25;
    ctx->x26_s10= 26;
    ctx->x27_s11= 27;
    ctx->x28_t3 = 28;
    ctx->x29_t4 = 29;
    ctx->x30_t5 = 30;
    ctx->x31_t6 = 31;

    ctx->mstatus = (3 << 11) | (1 << 7)|(0 << 3);  //1880
}

void os_plat_init(void)
{
    SystemCoreClockUpdate();

    SysTick->CTLR = 0;
    SysTick->SR   = 0;
    SysTick->CNT  = 0;
    SysTick->CMP  = SystemCoreClock/(1000 / OS_SYSTICK_MS);

    SysTick->CTLR = 0xf;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    NVIC_SetPriority(SysTicK_IRQn,0xe0);
    NVIC_SetPriority(Software_IRQn,0xf0);  //优先级要最低 

    NVIC_EnableIRQ(SysTicK_IRQn);
    NVIC_EnableIRQ(Software_IRQn);
}

os_isr_status_t os_plat_isr_disable(void)
{
    os_isr_status_t os_status_t = (os_isr_status_t) __get_MSTATUS();
    __set_MSTATUS(0x1880);
    return os_status_t;
}

void os_plat_isr_enable(os_isr_status_t os_status_t)
{
    __set_MSTATUS(os_status_t);
}


void sw_setpend(void)
{
    SysTick->CTLR |= (1 << 31);
}

void sw_clearpend(void)
{
    SysTick->CTLR &= ~ (1 << 31);
}

os_task_ctx_t *in_isr_ctx_from;
os_task_ctx_t *in_isr_ctx_to;

void os_task_switch_from_isr(os_task_ctx_t * from, os_task_ctx_t * to)
{
    in_isr_ctx_from = from;
    in_isr_ctx_to = to;
    sw_setpend(); 
}

void os_plat_ticks_update(os_tick_t * ticks,os_ticks_update_mode update_mode)
{
    switch (update_mode)
    {
        case OS_TICK_UPDATE_CLEAR:
            * ticks = 0;
            break;

        case OS_TICK_UPDATE_ADD:
            * ticks += 1;
            break;

        default:
            break;
    }
}

// void SysTick_Handler(void)
// {
//     os_plat_use_isr_sp_in();
//     os_isr_enter();

//     //static int tick_count = 0;
//     SysTick->SR = 0;
//     //printf("tick_count = %d\r\n",tick_count++);
//     os_sched_time_tick();

//     // os_core.os_sched_in_isr_flag =1;
//     // os_sched_run();

//     os_isr_leave();
//     os_plat_use_isr_sp_out();
// }



void SysTick_Handler(void)
{
    os_plat_use_isr_sp_in();

    os_isr_enter();

    SysTick->SR = 0;
    os_sched_time_tick();                     /* tick 处理主体 */

    os_isr_leave();

    os_plat_use_isr_sp_out();
}