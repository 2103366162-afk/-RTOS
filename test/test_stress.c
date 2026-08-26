/* ============================================================
 * RTOS 压力测试 & 边界测试
 * 测试项：
 *   1. 优先级反转场景复现（验证互斥量优先级继承）
 *   2. 信号量被高/低优先级任务反复抢占
 *   3. 内存分配/释放 10 万次后的碎片率
 *   4. 栈溢出触发 Guard 检测（故意写越界）
 *
 * 使用方法：在 main 里 os_sys_init() 后调用 stress_test_init()
 * 注意：测试4会故意触发 os_assert(0)，建议单独跑该项
 * ============================================================ */
#include "os_sys.h"
#include "os_men.h"
#include "os_mutex.h"
#include "os_sem.h"
#include <stdio.h>

/* ---------------- 测试开关 ---------------- */
#define STRESS_TEST_INVERSION   0   /* 优先级反转 */
#define STRESS_TEST_SEM         0   /* 信号量抢占 */
#define STRESS_TEST_MEM         0   /* 内存碎片 10万次 */
#define STRESS_TEST_STACK       1   /* 栈溢出(会死循环，单独跑) */

/* ---------------- 测试1：优先级反转 ----------------
 * 场景：LPT持有互斥量 -> HPT请求被阻塞 -> MPT抢占LPT
 * 若优先级继承生效：LPT被提升，快速释放，HPT等待时间短
 * 若失效：LPT被MPT无限抢占，HPT长时间等待（近似死锁）
 * 测量：HPT等待期间 MPT 空转计数器的增量，增量越小说明继承越好
 * --------------------------------------------------- */
#if STRESS_TEST_INVERSION
static os_mutex_t  stress_mutex;
static os_sem_t    stress_sem_holding;   /* HPT等待：LPT已持锁 */
static os_sem_t    stress_sem_start_mpt; /* MPT等待：LPT已持锁，可以来抢了 */
static os_task_t   stress_tcb_lpt, stress_tcb_mpt, stress_tcb_hpt;
static uint8_t     stress_stack_lpt[1024], stress_stack_mpt[1024], stress_stack_hpt[1024];

static volatile uint32_t stress_mpt_busy;        /* MPT空转计数 */
static volatile uint32_t stress_hpt_wait_cycles; /* HPT等待期间MPT执行量 */

/* 低优先级任务（20）：先持锁，放行MPT和HPT，长时间工作 */
static void stress_lpt_task(void *param)
{
    for (;;) {
        os_mutex_lock(&stress_mutex, 0);              /* ① 拿到锁 */

        os_sem_release(&stress_sem_start_mpt);        /* ② 放行MPT：可以来抢了 */
        os_sem_release(&stress_sem_holding);          /* ③ 通知HPT：我已持锁 */

        for (volatile int i = 0; i < 1000000; i++);   /* ④ 长时间持锁工作 */

        os_mutex_unlock(&stress_mutex);               /* ⑤ 释放锁 */
        os_task_delay(1);
    }
}

/* 中优先级任务（15）：先阻塞，等LPT持锁后才开始空转 */
static void stress_mpt_task(void *param)
{
    for (;;) {
        os_sem_take(&stress_sem_start_mpt, 0);        /* 等待LPT持锁信号 */
        stress_mpt_busy++;                            /* 空转霸占CPU */
    }
}

/* 高优先级任务（5）：等LPT持锁后请求锁，测量等待时间 */
static void stress_hpt_task(void *param)
{
    for (;;) {
        os_sem_take(&stress_sem_holding, 0);          /* 等LPT确认已持锁 */

        uint32_t t0 = stress_mpt_busy;
        os_mutex_lock(&stress_mutex, 0);              /* 必然被阻塞 */
        stress_hpt_wait_cycles = stress_mpt_busy - t0; /* 等待期间MPT跑了多少 */
        os_mutex_unlock(&stress_mutex);

        os_task_delay(1);

        static uint32_t print_cnt = 0;
        if (++print_cnt % 100 == 0) {
            os_printf("[inversion] HPT wait(MPT cycles)=%u\r\n",
                      (unsigned)stress_hpt_wait_cycles);
        }
    }
}
#endif

/* ---------------- 测试2：信号量反复抢占 ----------------
 * 3个任务抢同一个二值信号量，各自计数。
 * 验证：互斥性（总计数正确） + 无饿死（各任务都能执行）
 * --------------------------------------------------- */
#if STRESS_TEST_SEM
static os_sem_t  stress_sem;
static os_task_t stress_tcb_sa, stress_tcb_sb, stress_tcb_sc;
static uint8_t   stress_stack_sa[1024], stress_stack_sb[1024], stress_stack_sc[1024];
static volatile uint32_t stress_cnt_a, stress_cnt_b, stress_cnt_c;

static void stress_sem_task(void *param)
{
    int id = (int)param;
    for (;;) {
        os_sem_take(&stress_sem, 0);           /* 进入临界区 */
        if (id == 0)      stress_cnt_a++;
        else if (id == 1) stress_cnt_b++;
        else              stress_cnt_c++;
        os_sem_release(&stress_sem);           /* 退出临界区 */
        os_task_delay(1);                      /* 让出，避免饿死其他任务 */
        /* 每 1024 次打印一次状态 */
        if (((stress_cnt_a + stress_cnt_b + stress_cnt_c) & 0x3FF) == 0) {
            os_printf("[sem] a=%u b=%u c=%u total=%u\r\n",
                      (unsigned)stress_cnt_a, (unsigned)stress_cnt_b,
                      (unsigned)stress_cnt_c,
                      (unsigned)(stress_cnt_a + stress_cnt_b + stress_cnt_c));
        }
    }
}
#endif

/* ---------------- 测试3：内存碎片 10万次 ----------------
 * 随机分配/释放 16~207 字节块，共10万次操作
 * 统计：总空闲字节、最大连续块、空闲块数、碎片率
 * 碎片率 = 1 - (最大连续块 / 总空闲)  [1]
 * --------------------------------------------------- */
/* ============================================================
 * 内存碎片 / 合并压力测试（替换原来 STRESS_TEST_MEM 部分）
 *
 * 阶段1：随机分配/释放 10万次，结束后打印当前空闲状态（有些块仍占用）
 *       --- 用于观察真实负载下的碎片分布 ---
 * 阶段2：把剩余所有块释放干净，再打印空闲链表
 *       --- 如果分配器合并正确，空闲块数应接近1，碎片率应接近0% ---
 *
 * 注意：本测试依赖堆管理器 os_men_* 的实现，运行前确认：
 *       OS_MEN_EN=1, OS_MEN_INFO_EN=1, OS_LIST_INFO_EN=0(性能和正确性取平衡)
 * ============================================================ */
#if STRESS_TEST_MEM
#define STRESS_MEM_SLOTS     32
#define STRESS_MEM_MAX_OPS   100000

static void *stress_slots[STRESS_MEM_SLOTS];
static uint32_t stress_rand = 12345;

static uint32_t stress_next_rand(void)
{
    stress_rand = stress_rand * 1664525u + 1013904223u;   /* LCG随机数 */
    return stress_rand;
}

/* 打印当前堆空闲状态：总空闲、最大连续块、空闲块数、碎片率整数百分比 */
static void stress_mem_print_status(const char *tag)
{
    int total_free = 0, max_block = 0, blk_cnt = 0;

    os_list_for_each(curr, os_men_item_t, &os_core.heap_men.list) {
        total_free += curr->data_size;
        if (curr->data_size > max_block) max_block = curr->data_size;
        blk_cnt++;
    }

    int frag_pct = 0;
    if (total_free > 0) {
        frag_pct = 100 - (max_block * 100 / total_free);
    }

    os_printf("[mem] %s: free=%d maxblk=%d blks=%d frag=%d%%\r\n",
              tag, total_free, max_block, blk_cnt, frag_pct);
}

/* 阶段1：随机分配/释放10万次 */
static void stress_mem_random_ops(void)
{
    for (int i = 0; i < STRESS_MEM_MAX_OPS; i++) {
        int idx = stress_next_rand() % STRESS_MEM_SLOTS;

        if (stress_slots[idx] == NULL) {
            /* 分配一个 16~207 字节的块 */
            int size = 16 + (stress_next_rand() % 192);
            stress_slots[idx] = os_mem_malloc(size);
            if (stress_slots[idx] == NULL) {
                os_printf("[mem] malloc FAIL at op=%d size=%d\r\n", i, size);
            }
        } else {
            /* 释放一个已有块 */
            os_men_free(stress_slots[idx]);
            stress_slots[idx] = NULL;
        }

        /* 每10000次打印一次中间状态，便于观察碎片演化 */
        if ((i + 1) % 20000 == 0) {
            stress_mem_print_status("mid");
        }
    }

    /* 阶段1结束，此时仍有部分块被占用 */
    stress_mem_print_status("phase1 end (some blocks still alloc)");
}

/* 阶段2：释放所有剩余块，验证合并 */
static void stress_mem_free_all_and_check(void)
{
    for (int i = 0; i < STRESS_MEM_SLOTS; i++) {
        if (stress_slots[i] != NULL) {
            os_men_free(stress_slots[i]);
            stress_slots[i] = NULL;
        }
    }

    stress_mem_print_status("phase2 all freed");

    /* 额外验证：能否连续分配几个大块？如果合并正确，应该能成功分配大块 */
    void *big1 = os_mem_malloc(2048);
    void *big2 = os_mem_malloc(2048);
    os_printf("[mem] big alloc test: 2048->%s, 2048->%s\r\n",
              big1 ? "OK" : "FAIL", big2 ? "OK" : "FAIL");
    if (big1) os_men_free(big1);
    if (big2) os_men_free(big2);
}

/* 总入口 */
static void stress_mem_test(void)
{
    stress_mem_random_ops();
    stress_mem_free_all_and_check();
}
#endif /* STRESS_TEST_MEM */

/* ---------------- 测试4：栈溢出触发 Guard ----------------
 * 创建 256 字节小栈任务，故意声明 512 字节局部数组写越界
 * 预期：破坏 Guard 区(0x5A) -> 调度器检测到 -> os_assert(0)
 * 若打印 "NOT detected" 说明 Guard 失效
 * --------------------------------------------------- */
#if STRESS_TEST_STACK
static os_task_t stress_tcb_ovf;
static uint8_t   stress_stack_ovf[382];   /* 故意给很小的栈 */

static void stress_stack_overflow_task(void *param)
{
    /* 512字节局部数组 > 256字节栈，必然向下溢出破坏 Guard 区 */
    uint8_t big_buf[512];
    for (int i = 0; i < (int)sizeof(big_buf); i++) {
        big_buf[i] = 0x55;
    }
    /* 让出CPU，触发调度器对下一个任务的栈检查 */
    os_task_delay(1);
    /* 如果走到这里，说明 Guard 检测没生效 */
    os_printf("ERROR: stack overflow NOT detected!\r\n");
    for (;;) {}
}
#endif

/* ============================================================
 * 启动入口：os_sys_init() 之后调用
 * ============================================================ */
void stress_test_init(void)
{
#if STRESS_TEST_INVERSION
    os_mutex_create_static(&stress_mutex);
    os_sem_create_static(&stress_sem_holding, 0, 1);      /* HPT 等待：LPT已持锁 */
    os_sem_create_static(&stress_sem_start_mpt, 0, 1);    /* MPT 等待：LPT已持锁 */
    os_task_create_static(&stress_tcb_lpt, "stress_lpt", stress_lpt_task,
                          OS_NULL, 20, stress_stack_lpt, sizeof(stress_stack_lpt));
    os_task_create_static(&stress_tcb_mpt, "stress_mpt", stress_mpt_task,
                          OS_NULL, 15, stress_stack_mpt, sizeof(stress_stack_mpt));
    os_task_create_static(&stress_tcb_hpt, "stress_hpt", stress_hpt_task,
                          OS_NULL, 5,  stress_stack_hpt, sizeof(stress_stack_hpt));
    os_task_start(&stress_tcb_lpt);
    os_task_start(&stress_tcb_mpt);
    os_task_start(&stress_tcb_hpt);
#endif

#if STRESS_TEST_SEM
    os_sem_create_static(&stress_sem, 1, 1);
    os_task_create_static(&stress_tcb_sa, "stress_sa", stress_sem_task,
                          (void *)0, 10, stress_stack_sa, sizeof(stress_stack_sa));
    os_task_create_static(&stress_tcb_sb, "stress_sb", stress_sem_task,
                          (void *)1, 10, stress_stack_sb, sizeof(stress_stack_sb));
    os_task_create_static(&stress_tcb_sc, "stress_sc", stress_sem_task,
                          (void *)2, 10, stress_stack_sc, sizeof(stress_stack_sc));
    os_task_start(&stress_tcb_sa);
    os_task_start(&stress_tcb_sb);
    os_task_start(&stress_tcb_sc);
#endif

#if STRESS_TEST_MEM
    /* 内存测试在主循环前跑一次（也可单独开一个任务反复跑） */
    stress_mem_test();
#endif

#if STRESS_TEST_STACK
    os_task_create_static(&stress_tcb_ovf, "stress_ovf", stress_stack_overflow_task,
                          OS_NULL, 8, stress_stack_ovf, sizeof(stress_stack_ovf));
    os_task_start(&stress_tcb_ovf);
#endif
}