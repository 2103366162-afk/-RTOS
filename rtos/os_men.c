#include "os_def.h"
#include "os_err.h"
#include "os_sched.h"
#include "os_dbg.h"
#include <stdbool.h>

#if !OS_MEN_DBG_PRINTF_EN
#undef os_dbg
#define os_dbg(fmt,...)   do{}while(0);
#endif

#if OS_MEN_EN
static uint8_t heap_mem[OS_MEN_ALLOC_SIZE];
extern os_core_t os_core;

static bool men_item_can_merge(os_men_item_t * pre,os_men_item_t * after)
{
    bool merge = 0;
    merge = ((uint8_t *)pre + pre->data_size + OS_MEN_ITEM_SIZE  == (uint8_t * )after);
    return merge;
}

static void men_item_merge(os_men_item_t * pre,os_men_item_t * after)
{
    pre->data_size += after->data_size + OS_MEN_ITEM_SIZE; 
}

static void os_men_item_init(os_men_item_t * men_item,uint32_t data_size)
{
    men_item->used = 0;
    men_item->data_size = data_size;
    os_list_item_init(&men_item->item,os_men_item_t,item);
}

static void os_insert_to_men_list(os_men_item_t * men_item)
{
    if(os_list_count(&os_core.heap_men.list) == 0)
    {
        os_list_insert_first(&os_core.heap_men.list, men_item);
        return;
    }

    os_list_for_each(high_item, os_men_item_t, &os_core.heap_men.list)
    {
        if((uint8_t *)high_item >= (uint8_t *)men_item)
        {
            break;   /* 找到第一个地址高于新块的节点 */
        }
    }

    if(high_item == OS_NULL)
    {
        /* 插入到链表末尾，尝试与最后一个合并 */
        os_men_item_t * last_item = os_list_end(&os_core.heap_men.list);
        if(men_item_can_merge(last_item, men_item))
        {
            os_list_remove_item(&os_core.heap_men.list, last_item);
            men_item_merge(last_item, men_item);
            men_item = last_item;
            os_list_insert_last(&os_core.heap_men.list, men_item);
        }
        else
        {
            os_list_insert_last(&os_core.heap_men.list, men_item);
        }
    }
    else
    {
        os_men_item_t * pre_item = os_list_item_pre(&os_core.heap_men.list, high_item);

        /* ============ 修复点：先保存前驱的前驱，再移除 ============ */
        if(pre_item != OS_NULL)
        {
            if(men_item_can_merge(pre_item, men_item))
            {
                os_men_item_t * pre_pre_item = os_list_item_pre(&os_core.heap_men.list, pre_item);  /* ① 移除前先保存 */
                os_list_remove_item(&os_core.heap_men.list, pre_item);
                men_item_merge(pre_item, men_item);
                men_item = pre_item;
                pre_item = pre_pre_item;   /* ② 用保存的值，而不是被清空的节点 */
            }
        }

        /* 与 high_item 合并 */
        if(men_item_can_merge(men_item, high_item))
        {
            os_list_remove_item(&os_core.heap_men.list, high_item);
            men_item_merge(men_item, high_item);
            if(pre_item != OS_NULL)
            {
                os_list_insert_after(&os_core.heap_men.list, pre_item, men_item);
            }
            else
            {
                os_list_insert_first(&os_core.heap_men.list, men_item);
            }
        }
        else
        {
            os_list_insert_after(&os_core.heap_men.list, pre_item, men_item);
        }
    }
}

os_err_t os_men_init(void)
{
    /*os_dbg里的此时这里的heap_end，指在heap_mem有效的最后一个字节的地址，不是指结束边界的地址
      结束边界的地址应该是heap_end +1*/
    os_dbg("aliged heap_mem start adder : %x, heap_mem end adder : %x,size: %d\r\n",(int)heap_mem,(int)&heap_mem[OS_MEN_ALLOC_SIZE-1],OS_MEN_ALLOC_SIZE);
    /*起点向高地址对齐此时这里的heap_start，指在heap_mem有效的第一个字节的地址，不是指开始边界的地址
      开始边界的地址应该是heap_start - 1*/
    uint8_t * heap_start = (uint8_t *)MEN_HIGH_ALIGED(heap_mem,MEN_ALLGN_BYTES);  
    /*终点向低地址对齐，此时这里的heap_end，指在heap_mem有效的最后一个字节的地址，不是指结束边界的地址
      结束边界的地址应该是heap_end +1*/
    uint8_t * heap_end = (uint8_t *)MEN_LOW_ALIGED(heap_mem + OS_MEN_ALLOC_SIZE,MEN_ALLGN_BYTES) -1; 
    
    os_dbg("aliged heap_mem start adder : %x, heap_mem end adder : %x,size: %d\r\n",(int)heap_start,(int)heap_end,(int)(heap_end-heap_start+1));

    os_list_init(&os_core.heap_men.list,os_men_item_t,item);

    os_core.heap_men.heap_statck = heap_start;
    os_core.heap_men.heap_size = heap_end - heap_start +1;
    os_core.heap_men.malloc_count = 0;
    /*heap_start已经进行4字节对齐，后续访问first_item指向的内存就不会出现非4对齐的地址访问，导致的HardFault*/
    os_men_item_t * first_item = (os_men_item_t * ) heap_start ;

    /*这里的data_size是指空闲块的字节数，是不包含os_men_item_t的*/
    os_men_item_init(first_item,OS_MEN_ALLOC_SIZE - OS_MEN_ITEM_SIZE);
    os_list_insert_first(&os_core.heap_men.list,first_item);
    
    return OS_ERR_OK;
}

void * os_mem_malloc(int size)
{
    os_param_failed(size == 0 , OS_NULL);
    
    int request_size = MEN_HIGH_ALIGED(size,MEN_ALLGN_BYTES);
    void * free_start_adder = OS_NULL;

    os_list_for_each(curr,os_men_item_t,&os_core.heap_men.list)
    {
        if(curr->data_size >= request_size)
        {
            break;
        }
    }

    if(curr != OS_NULL)
    {   
        os_list_remove_item(&os_core.heap_men.list,curr);
        
        uint16_t remaining_heap_size = curr->data_size - request_size  ;
        
        if(remaining_heap_size > OS_MEN_ITEM_SIZE + OS_MEN_MIN_SIZE )
        {
            os_men_item_t * Separate_item = (os_men_item_t * ) ( (uint8_t *)curr + OS_MEN_ITEM_SIZE + request_size);
            os_men_item_init(Separate_item,remaining_heap_size - OS_MEN_ITEM_SIZE);
            os_insert_to_men_list(Separate_item);
            curr->data_size = request_size;  
            os_core.heap_men.malloc_count ++;
        }
        os_list_item_init(&curr->item,os_men_item_t,item);
        curr->used = 1;
        /*curr和OS_MEN_ITEM_SIZE都进行4字节对齐了 所以free_start_adder一定是4字节对齐的*/
        free_start_adder = (uint8_t *)curr + OS_MEN_ITEM_SIZE;

        os_dbg("alloc memory start: %x, input size: %d, allocated size: %d\r\n",(int)free_start_adder,size,curr->data_size);
    }else
    {
        os_dbg("malloc failed, men to small\r\n");
    }
    return (void *)free_start_adder;
}

os_err_t os_men_free(void * men)
{
    os_param_failed(men == OS_NULL,OS_ERR_PARAM);
    os_dbg("free: %x \r\n", (int)men);

    os_men_item_t * men_item = (os_men_item_t *)((uint8_t *)men - OS_MEN_ITEM_SIZE) ;
    os_assert(men_item->used == 1);
    os_assert(men_item->data_size  >= OS_MEN_MIN_SIZE);
   // os_assert(men_item->item.next != OS_NULL && men_item->item.pre != OS_NULL);

    if((men_item->used == 1 ) && (men_item->data_size < OS_MEN_MIN_SIZE))
    {
        return OS_ERR_PARAM;
    }
    men_item->used = 0;
    os_core.heap_men.malloc_count --;
    os_insert_to_men_list(men_item);
    return OS_ERR_OK;
}

#if OS_MEN_INFO_EN
int os_get_free_size(void)
{
    int count = 0;
    os_list_for_each(curr,os_men_item_t,&os_core.heap_men.list)
    {
        count += curr->data_size;
    }
    return count;
}
#endif

#endif