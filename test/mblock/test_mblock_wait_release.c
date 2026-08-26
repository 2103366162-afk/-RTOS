
#include "os_sys.h"

static cpu_stack_t task1_stack[200];
static cpu_stack_t task2_stack[200];
// static cpu_stack_t task3_stack[200];
// static cpu_stack_t task4_stack[200];
static os_task_t task1;
static os_task_t task2;
// static os_task_t task3;
// static os_task_t task4;

#define MEM_BLK_SIZE		11
#define MEM_BLK_CNT			4

static os_mblock_t * mblock;
// static uint8_t mem_blks[MEM_BLK_CNT * MEM_BLK_SIZE];

void task1_entry (void * arg) {
    //int cnt = 0;

	void * mem[MEM_BLK_CNT];
	os_printf("\r\n");
	// 先把所有的申请完毕
	for (int i = 0; i < MEM_BLK_CNT; i++) {
		mem[i] = os_mblock_wait(mblock, 0, OS_NULL);
		os_printf("task1: get mem block: %x\r\n",(int) mem[i]);
		os_printf("task1: mem block cnt: %d, data_size: %d, wait: %d\r\n",
		                    os_mblock_blk_cnt(mblock),
							os_mblock_blk_size(mblock),
							os_mblock_tasks(mblock));
	}

	// 让task2运行, 从而申请内存块
	os_task_delay(150);
	os_printf("\r\n");
	for (int i = 0; i < MEM_BLK_CNT; i++) {
	    os_printf("task1: before realase: mem block cnt: %d, data_size: %d, wait: %d\r\n",
	                        os_mblock_blk_cnt(mblock),
							os_mblock_blk_size(mblock),
							os_mblock_tasks(mblock));
	    os_printf("task1: free mem block: %x\r\n", (int)mem[i]);
		os_mblock_release(mblock, mem[i]);
		os_task_delay(20);
	}
	os_printf("\r\n");
	for (;;) {
		//os_printf("task1 is running, %d\n", ++cnt);
        os_task_delay(1000);
	}
}

void task2_entry (void * arg) {
    //int cnt = 0;
	
	// 先停一会儿，保证task1能先运行
	os_task_delay(100);
	os_printf("\r\n");
	// 申请内存块，这里会等待task1释放
	for (int i = 0; i < MEM_BLK_CNT; i++) {
		void * msg = os_mblock_wait(mblock, 1000, OS_NULL);
		os_printf("task2: get mem block: %x\r\n",(int) msg);
		os_printf("task2: mem block cnt: %d, data_size: %d, wait: %d\r\n",
		                os_mblock_blk_cnt(mblock),
						os_mblock_blk_size(mblock),
						os_mblock_tasks(mblock));
	}
	os_printf("\r\n");

	for (;;) {
		//os_printf("task2 is running, %d\n", ++cnt);
        os_task_delay(1000);
	}
}

void task3_entry (void * arg) {
    //int cnt = 0;

	for (;;) {
		//os_printf("task3 is running, %d\n", ++cnt);
        os_task_delay(1000);
	}
}

void task4_entry (void * arg) {
    //int cnt = 0;

	for (;;) {
        //os_printf("task4 is running, %d\n", ++cnt);
        os_task_delay(1000);
	}
}

void os_mblock_wait_release_test (void) {
	mblock = os_mblock_create(MEM_BLK_SIZE, MEM_BLK_CNT);

	os_err_t err;
	err = os_task_create_static(&task1, "task1", task1_entry, OS_NULL,
	                0, task1_stack, sizeof(task1_stack));
	os_assert(err == OS_ERR_OK);
	os_task_start(&task1);

    err = os_task_create_static(&task2, "task2", task2_entry, OS_NULL,
	                0, task2_stack, sizeof(task2_stack));
    os_assert(err == OS_ERR_OK);
	os_task_start(&task2);

	// err = os_task_create_static(&task3, "task3", task3_entry, OS_NULL,
	//                 0, task3_stack, sizeof(task3_stack));
    // os_assert(err == OS_ERR_OK);
	// os_task_start(&task3);

    // err = os_task_create_static(&task4, "task4", task4_entry, OS_NULL,
	//                 0, task4_stack, sizeof(task4_stack));
    // os_assert(err == OS_ERR_OK);
	// os_task_start(&task4);
}

