/* ============================================================
 * 性能测试任务：上下文切换 / 调度延迟 / tick 开销
 * 测试引脚：PB0 -> SLogic CH0
 *          PB1 -> SLogic CH1（测 tick 时用）
 * 使用前请确认 PB0/PB1 未被 UART4 或板载外设占用！
 * 测试前请关闭 os_dbg 打印、参数检查、栈检查，并开 -O2
 * ============================================================ */
#include "os_sys.h"
#include "os_sem.h"
#include "ch32v20x.h"

/* ---------------- 测试开关：一次只开一个 ---------------- */
#define BENCH_TEST_SWITCH   0    /* 1: 上下文切换(乒乓方波) */
#define BENCH_TEST_DISPATCH 0    /* 1: 调度延迟(ISR->任务)  */
#define BENCH_TEST_TICK     1    /* 1: tick开销(需改os_plat.c) */

/* ---------------- GPIO 初始化与操作宏（PB0/PB1） ---------------- */
static void bench_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

#define BENCH_GPIO_HIGH(bit)   (GPIOB->BSHR = (uint32_t)(1u << (bit)))
#define BENCH_GPIO_LOW(bit)    (GPIOB->BSHR = (uint32_t)(1u << ((bit)+16)))
#define BENCH_GPIO_TOGGLE(bit) (GPIOB->OUTDR ^= (uint32_t)(1u << (bit)))

#if BENCH_TEST_SWITCH
/* ============================================================
 * 测试1：上下文切换耗时（乒乓方波法）
 * 两个同优先级任务，每次切换翻转一次 PB0，输出方波：
 *   切换总耗时(含yield+调度) = 方波周期 / 2
 * 逻辑分析仪测 PB0 频率 -> 取倒数得周期 -> 除以 2
 * ============================================================ */
static os_task_t bench_tcb_a;
static uint8_t   bench_stack_a[1024];
static os_task_t bench_tcb_b;
static uint8_t   bench_stack_b[1024];


static void bench_switch_task_a(void *param)
{
    for (;;) {
        BENCH_GPIO_TOGGLE(0);   /* 每次切换翻转一次 */
        os_sched_yield();       /* 让出CPU，触发调度+切换 */
    }
}

static void bench_switch_task_b(void *param)
{
    for (;;) {
        BENCH_GPIO_TOGGLE(0);
        os_sched_yield();
    }
}
#endif /* BENCH_TEST_SWITCH */

#if BENCH_TEST_DISPATCH
/* ============================================================
 * 测试2：调度延迟（ISR 释放信号量 -> 高优先级任务真正执行）
 * 结构：高优先级任务阻塞在信号量；低优先级任务空转；
 *       TIM2 周期中断里：置高PB0 -> 释放信号量 -> 退出中断时切换；
 *       高优先级任务第一行代码拉低 PB0。
 *  PB0 脉冲宽度 = 调度延迟（dispatch latency）
 * ============================================================ */
static os_sem_t bench_sem;
static os_task_t bench_tcb_high;
static uint8_t   bench_stack_high[1024];
static os_task_t bench_tcb_low;
static uint8_t   bench_stack_low[1024];

static void bench_dispatch_high(void *param)
{
    for (;;) {
        os_sem_take(&bench_sem, 0);   /* 永久等待信号量 */
        BENCH_GPIO_LOW(0);            /* 任务真正开始执行 -> 结束计时 */
        //os_task_delay(1);             /* 让出，给低优先级任务运行机会 */
    }
}
static void bench_tim2_init(void);
static void bench_dispatch_low(void *param)
{
    bench_tim2_init();
    for (;;) 
    {
    
    }                      /* 空转，模拟低优先级任务正在运行 */
}

/* TIM2 中断：必须在 os_isr_enter/leave 之间调用 os_sem_release */
void TIM2_IRQHandler(void)__attribute__((interrupt()));
void TIM2_IRQHandler(void)
{
    os_plat_use_isr_sp_in();
    os_isr_enter();

    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
        BENCH_GPIO_HIGH(0);
        os_sem_release_from_isr(&bench_sem);   // 修改为中断安全版本
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }

    os_isr_leave();

    os_plat_use_isr_sp_out();
}

/* TIM2 初始化：周期约271us（与10ms tick互质，避免相位锁定）
 * 假设 TIM2 时钟 = 144MHz：PSC=143 -> 1MHz计数，ARR=270 -> 271us */
static void bench_tim2_init(void)
{
    TIM_TimeBaseInitTypeDef TIM_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    TIM_InitStructure.TIM_Period        = 271;
    TIM_InitStructure.TIM_Prescaler     = 143;
    TIM_InitStructure.TIM_ClockDivision = 5;
    TIM_InitStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_InitStructure);

    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel                   = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;  /* 低于SysTick(0) */
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    TIM_Cmd(TIM2, ENABLE);
}
#endif /* BENCH_TEST_DISPATCH */

/* ============================================================
 * 启动测试
 * ============================================================ */
void bench_test_init(void)
{
    bench_gpio_init();

#if BENCH_TEST_SWITCH
    os_task_create_static(&bench_tcb_a, "bench_a", bench_switch_task_a,
                          OS_NULL, 2, bench_stack_a, sizeof(bench_stack_a));
     os_task_create_static(&bench_tcb_b, "bench_b", bench_switch_task_b,
                           OS_NULL, 2, bench_stack_b, sizeof(bench_stack_b));
    os_task_start(&bench_tcb_a);
    os_task_start(&bench_tcb_b);
#endif

#if BENCH_TEST_DISPATCH
    os_sem_init(&bench_sem, 0, 1);
    os_task_create_static(&bench_tcb_high, "bench_high", bench_dispatch_high,
                          OS_NULL, 1, bench_stack_high, sizeof(bench_stack_high));
    os_task_create_static(&bench_tcb_low,  "bench_low",  bench_dispatch_low,
                          OS_NULL, 3, bench_stack_low, sizeof(bench_stack_low));
    os_task_start(&bench_tcb_high);
    os_task_start(&bench_tcb_low);
    //bench_tim2_init();
#endif
}