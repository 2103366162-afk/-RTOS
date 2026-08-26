#include "os_mblock.h"
#include "os_err.h"
#include "os_event.h"
#include "os_sched.h"
#include "os_dbg.h"
#include "os_men.h"

#if OS_MBLOCK_EN

#if !OS_MBLOCK_DBG_PRINT_EN
#undef os_dbg
#define os_dbg(fmt, ...) do {}while (0)
#endif

extern os_core_t os_core;

os_err_t mblock_init(os_mblock_t * mblock, void * mem,
                     uint16_t blk_size, uint16_t blk_cnt, int flag)
{
    os_err_t err = os_event_init(&mblock->event, OS_EVENT_TYPE_MBLOCK, flag);
    if (err < 0) {
        os_dbg("create mblock event init faied\r\n");
        return err;
    }

    mblock->men_start = mem;
    mblock->blk_size  = blk_size;      // 保存块大小
    mblock->blk_total = blk_cnt;       // 保存总块数
    mblock->blk_free  = blk_cnt;       // 空闲块数初始化 = 总块数

    uint8_t * mem_byte = (uint8_t *)mem;   // 用 uint8_t* 做字节算术
    os_men_lick_t * prev = OS_NULL;

    for (int i = 0; i < mblock->blk_free; i++)
    {
        os_men_lick_t * blk = (os_men_lick_t *)(mem_byte + i * blk_size);

        if (prev == OS_NULL) {
            mblock->list = blk;        // 第一个块挂到链表头
        } else {
            prev->next = blk;               // 上一个块指向当前块
        }
        prev = blk;   /*这里prev需要4字节对齐 否则会触发HardFault_Handler，这里采用
                        把blk_size进行4字节对齐，扩大对齐*/
    }

    if (prev != OS_NULL) 
    {
        prev->next = OS_NULL;               // 最后一个块 next 置空
    }

    return OS_ERR_OK;
}

os_err_t os_mblock_create_static (os_mblock_t * mblock, void * mem, uint16_t blk_size, uint16_t blk_free)
{
    os_param_failed(mblock == OS_NULL,OS_ERR_PARAM);
    os_param_failed(mem == OS_NULL,OS_ERR_PARAM);
    os_param_failed(blk_size <= sizeof(os_men_lick_t),OS_ERR_PARAM);
    os_param_failed(blk_free <= 0,OS_ERR_PARAM);

    int totoal_size = blk_size * blk_free;

    /*这里是把blk_size进行向上4字节对齐，为了避免超出数组缓冲区的范围，所以只能减小分配的个数
      实际分配出来的个数是<=要的个数，数组使用率在<=1*/
    blk_size = MEN_HIGH_ALIGED(blk_size,MEN_ALLGN_BYTES);
    blk_free = totoal_size / blk_size;
    os_param_failed(blk_free<=0,OS_ERR_PARAM);
    
    return mblock_init(mblock,mem,blk_size,blk_free,0);
}

os_err_t os_mblock_uninit (os_mblock_t * mblock)
{
    os_isr_status_t status =  os_sched_isr_disable();
    int cnt = os_event_wait_cnt(&mblock->event);
    os_event_uninit(&mblock->event);
    if(cnt > 0)
    {
        os_sched_run();
    }

    os_sched_isr_enable(status);

    return OS_ERR_OK;
}

os_mblock_t * os_mblock_create (uint16_t blk_size, uint16_t blk_free)
{
    os_param_failed(blk_free<=0, OS_NULL);
    os_param_failed(blk_size<sizeof(os_men_lick_t),OS_NULL);
    /*分配mblock结构体*/
    os_mblock_t * mblock = os_mem_malloc(sizeof(os_mblock_t));
    if(mblock == OS_NULL)
    {
        os_dbg("error:mblock mem_malloc failed");
        return  OS_NULL;
    }

    /*分配mblock的区域结构体*/
    blk_size = MEN_HIGH_ALIGED(blk_size,MEN_ALLGN_BYTES);
    void * blk_men_start = os_mem_malloc(blk_size * blk_free );
    if(blk_men_start == OS_NULL)
    {
        os_dbg("error:blk_men mem_malloc failed");
        os_men_free(mblock);
        return OS_NULL;
    }
    /* 因为是动态分配，所以不用考虑分配完超过原先设定的总数
       在os_mem_malloc也是按照size = MEN_HIGH_ALIGED(blk_size,MEN_ALLGN_BYTES);
       进行分配的*/

    os_err_t err = mblock_init(mblock,blk_men_start,blk_size,blk_free,OS_FLAG_MEM_HEAP);
    if(err<0)
    {
        os_dbg("error:mblock init failed");
        os_men_free(mblock);
        os_men_free(blk_men_start);
        return OS_NULL;
    }
    return  mblock;
}

os_err_t os_mblock_free (os_mblock_t * mblock)
{
    os_param_failed(mblock == OS_NULL,OS_ERR_PARAM);

    os_mblock_uninit(mblock);

    if(mblock->event.flag & OS_FLAG_MEM_HEAP)
    {
        os_men_free(mblock->men_start);
        os_men_free(mblock);
    }

    return  OS_ERR_OK;
}

void * os_mblock_wait(os_mblock_t * mblock, int ms, os_err_t * p_err)
{
    // os_param_failed_exec(mblock == OS_NULL,OS_NULL,);
    os_isr_status_t status =  os_sched_isr_disable();

    void * men = OS_NULL;


    if(mblock->blk_free != 0)
    {
        os_men_lick_t * men_mblk = mblock->list;
        men= (void *)men_mblk;
        
        mblock->list = men_mblk->next;
        mblock->blk_free --;

        os_sched_isr_enable(status);
        if(p_err != OS_NULL)
        {
            *p_err = OS_ERR_OK;
        }
        
        
    }else if((ms < 0) || (os_core.os_sched_in_isr_flag > 0))
    {
        os_sched_isr_enable(status);
        os_dbg("no mblk and no wait\r\n");
        if(p_err != OS_NULL)
        {
            *p_err = OS_ERR_NONE;
        }
    }else
    {
        os_event_wait(&mblock->event,OS_NULL,ms);
        os_sched_isr_enable(status);
        os_sched_run();
        os_task_t * task = os_task_self();
        
        if(p_err != OS_NULL)
        {
            *p_err = task->event_info.err;
        }

        if(task->event_info.err == OS_ERR_OK)
        {
            men = task->event_info.reason;
        }
        
    }
    return men;
}

os_err_t os_mblock_release (os_mblock_t * mblock, void * mem)
{
    os_param_failed(mblock == OS_NULL, OS_ERR_PARAM);
    os_param_failed(mem == OS_NULL, OS_ERR_PARAM);

    uint8_t * base = (uint8_t *)mblock->men_start;
    uint8_t * addr = (uint8_t *)mem;

    // 1. 范围检查（使用总块数）
    uint32_t pool_size = (uint32_t)mblock->blk_total * mblock->blk_size;
    os_param_failed(addr < base, OS_ERR_PARAM);
    os_param_failed(addr >= base + pool_size, OS_ERR_PARAM);

    // 2. 对齐检查
    os_param_failed(((addr - base) % mblock->blk_size) != 0, OS_ERR_PARAM);

    // 3. 临界区
    os_isr_status_t status = os_sched_isr_disable();

    os_task_t * task = os_event_notify(&mblock->event);

    os_men_lick_t * men_blk = (os_men_lick_t *)addr;
    os_men_lick_t * cur = mblock->list;
    os_men_lick_t * pre = OS_NULL;

    if(task == OS_NULL)
    {
        // 按地址顺序寻找插入位置，同时检查是否重复释放
        while (cur != OS_NULL && cur < men_blk) 
        {
            pre = cur;
            cur = cur->next;
        }

        // 如果释放的块已经在链表中（地址相等），说明重复释放
        os_param_failed_exec(cur == men_blk, OS_ERR_PARAM, os_sched_isr_enable(status));

        // 插入链表
        if (pre == OS_NULL) 
        {
            men_blk->next = mblock->list;
            mblock->list = men_blk;
        } else {
            pre->next = men_blk;
            men_blk->next = cur;
        }

        mblock->blk_free++;
        os_sched_isr_enable(status);
    }else
    {
        task->event_info.err = OS_ERR_OK;
        task->event_info.reason = (void *)men_blk;
        os_sched_isr_enable(status);

        os_sched_run();
    }

    return OS_ERR_OK;
}

#if OS_MBLOCK_INFO_EN
uint16_t os_mblock_blk_cnt (os_mblock_t * mblock)
{
    os_param_failed(mblock == OS_NULL,0);

    os_isr_status_t status =  os_sched_isr_disable();
    uint16_t cnt = mblock->blk_total;
    os_sched_isr_enable(status);

    return cnt;
}

uint16_t os_mblock_blk_free (os_mblock_t * mblock)
{
    os_param_failed(mblock == OS_NULL,0);

    os_isr_status_t status =  os_sched_isr_disable();
    uint16_t blk_free = mblock->blk_free;
    os_sched_isr_enable(status);
    return blk_free;


}

uint16_t os_mblock_blk_size (os_mblock_t * mblock)
{
    os_param_failed(mblock == OS_NULL,0);

    os_isr_status_t status =  os_sched_isr_disable();
    uint16_t blk_size = mblock->blk_size;
    os_sched_isr_enable(status);
    return blk_size;
}

uint16_t os_mblock_tasks(os_mblock_t * mblock)
{
    os_param_failed(mblock == OS_NULL,0);

    os_isr_status_t status =  os_sched_isr_disable();
    uint16_t task_cnt = os_event_wait_cnt(&mblock->event);
    os_sched_isr_enable(status);
    return task_cnt;
}
#endif
#endif