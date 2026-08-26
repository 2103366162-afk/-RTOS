#include "os_queue.h"
#include "os_err.h"
#include "os_sched.h"
#include <string.h>

#if OS_QUEUE_EN

#if !OS_QUEUE_DBG_PRINT_EN
#undef os_dbg
#define os_dbg(fmt, ...) do {}while (0)
#endif

extern os_core_t os_core;

typedef struct _os_queue_wait_t
{
    void * msg;
    int opt;
}os_queue_wait_t;

static os_err_t queue_init(os_queue_t * queue, void * msg_buf, uint16_t msg_size, uint16_t msg_cnt,int flag)
{
    os_err_t err_read = os_event_init(&queue->read_event,OS_EVENT_TYPE_QUEUE,flag);
    if(err_read<0)
    {
        os_dbg("queue read event init faied");
        return  err_read;
    }

    os_err_t err_write = os_event_init(&queue->write_event,OS_EVENT_TYPE_QUEUE,flag);
    if(err_write<0)
    {
        os_dbg("queue write event init faied");
        os_event_uninit(&queue->read_event);
        return  err_write;
    }

    queue->queue_pool_start = msg_buf;
    queue->msg_size = msg_size;
    queue->total_size = msg_size * msg_cnt;

    queue->msg_cnt = 0;
    queue->msg_max_cnt = msg_cnt;

    queue->read = 0;
    queue->write = 0;

    return OS_ERR_OK;    
}

os_err_t os_queue_create_static (os_queue_t * queue, void * msg_buf, uint16_t msg_size, uint16_t msg_cnt)
{
    os_param_failed(queue == OS_NULL,OS_ERR_PARAM);
    os_param_failed(msg_buf == OS_NULL,OS_ERR_PARAM);
    os_param_failed(msg_size < 0,OS_ERR_PARAM);
    os_param_failed(msg_cnt < 0,OS_ERR_PARAM);

    return queue_init(queue,msg_buf,msg_size,msg_cnt,0);
}

os_err_t os_queue_uninit(os_queue_t * queue)
{
    os_param_failed(queue == 0,OS_ERR_PARAM);

    os_isr_status_t status =  os_sched_isr_disable();
    int cnt_read = os_event_wait_cnt(&queue->read_event);
    os_event_uninit(&queue->read_event);

    int cnt_write = os_event_wait_cnt(&queue->write_event);
    os_event_uninit(&queue->write_event);

    if(cnt_read > 0 || cnt_write > 0)
    {
        os_sched_run();
    }
    os_sched_isr_enable(status);

    return OS_ERR_OK;
}

os_queue_t * os_queue_create (uint16_t msg_size, uint16_t msg_cnt)
{
    os_param_failed(msg_size<=0, OS_NULL);
    os_param_failed(msg_cnt<=0,OS_NULL);

    /*分配queue结构体空间*/
    os_queue_t * queue = os_mem_malloc(sizeof(os_queue_t));
    if(queue == OS_NULL)
    {
        os_dbg("error:queue mem_malloc failed");
        return  OS_NULL;
    }

    /*分配queue的消息内存池空间,有进行4字节对齐处理*/
    msg_size = MEN_HIGH_ALIGED(msg_size,MEN_ALLGN_BYTES);
    void * queue_pool_start = os_mem_malloc(msg_size * msg_cnt);
    if(queue_pool_start == OS_NULL)
    {
        os_dbg("error:queue_pool_start mem_malloc failed");
        os_men_free(queue);
        return OS_NULL;
    }

    os_err_t err = queue_init(queue,queue_pool_start,msg_size,msg_cnt,OS_FLAG_MEM_HEAP);
    if(err<0)
    {
        os_dbg("error:queue init failed");
        os_men_free(queue);
        os_men_free(queue_pool_start);
        return OS_NULL;
    }

    return queue;
}

os_err_t os_queue_free (os_queue_t * queue)
{
    os_param_failed(queue == OS_NULL,OS_ERR_PARAM);

    os_queue_uninit(queue);

    if((queue->read_event.flag & OS_FLAG_MEM_HEAP) && (queue->write_event.flag & OS_FLAG_MEM_HEAP))
    {
        os_men_free(queue);
        os_men_free(queue->queue_pool_start);
    }

    return  OS_ERR_OK;
}
#if OS_QUEUE_INFO_EN
uint16_t os_queue_msg_cnt (os_queue_t * queue)
{
    os_param_failed(queue == OS_NULL,OS_ERR_PARAM);
    
    os_isr_status_t status =  os_sched_isr_disable();
    uint16_t cnt = queue->msg_cnt;
    os_sched_isr_enable(status);
    return cnt;   
}

uint16_t os_queue_free_cnt (os_queue_t * queue)
{
    os_param_failed(queue == OS_NULL,OS_ERR_PARAM);
    
    os_isr_status_t status =  os_sched_isr_disable();
    uint16_t cnt = queue->msg_max_cnt - queue->msg_cnt;
    os_sched_isr_enable(status);
    return cnt; 
}

uint16_t os_queue_read_task_cnt(os_queue_t * queue)
{
    os_param_failed(queue == OS_NULL,OS_ERR_PARAM);

    os_isr_status_t status =  os_sched_isr_disable();
    uint16_t task_cnt = os_event_wait_cnt(&queue->read_event);
    os_sched_isr_enable(status);
    return task_cnt;
}

uint16_t os_queue_write_task_cnt (os_queue_t * queue)
{
    os_param_failed(queue == OS_NULL,OS_ERR_PARAM);

    os_isr_status_t status =  os_sched_isr_disable();
    uint16_t task_cnt = os_event_wait_cnt(&queue->write_event);
    os_sched_isr_enable(status);
    return task_cnt;
}

void os_queue_show_status (os_queue_t * queue) {
	os_printf("queue: msg=%d, free=%d, read task=%d, write task=%d\r\n",
			os_queue_msg_cnt(queue), os_queue_free_cnt(queue),
			os_queue_read_task_cnt(queue), os_queue_write_task_cnt(queue));
}
#endif

os_err_t os_queue_clear (os_queue_t * queue)
{
    os_param_failed(queue == OS_NULL,OS_ERR_PARAM);
    
    os_isr_status_t status =  os_sched_isr_disable();

    os_event_notify_all(&queue->write_event,OS_NULL,OS_ERR_QUEUE_CLAER);
    os_event_notify_all(&queue->read_event,OS_NULL,OS_ERR_QUEUE_CLAER);
    queue->msg_cnt = 0;
    queue->read = 0;
    queue->write = 0;
    
    os_sched_isr_enable(status);
    return OS_ERR_OK;    
}

static void queue_read_msg (os_queue_t * queue, int opt, void * msg)
{
    memcpy(msg,queue->queue_pool_start + queue->read,queue->msg_size);
        queue->read += queue->msg_size;
        if(queue->read >= queue->total_size)
        {
            queue->read = 0;
        }
        queue->msg_cnt --;    
}

static void queue_wirte_in (os_queue_t * queue, int opt, void * msg)
{
    if(opt == OS_QUEUE_RELESE_UPGENT)
    {
        if(queue->read <= 0)
        {
            queue->read = queue->total_size - queue->msg_size;
        }else 
        {
            queue->read -= queue->msg_size;
        }
        memcpy((uint8_t *) queue->queue_pool_start + queue->read,msg,queue->msg_size);
        queue->msg_cnt ++;
    }else if(opt == OS_QUEUE_RELESE_NORMAL)
    {
        memcpy((uint8_t *) queue->queue_pool_start + queue->write,msg,queue->msg_size);
        queue->write +=queue->msg_size;
        if(queue->write >= queue->total_size)
        {
            queue->write = 0;
        }
        queue->msg_cnt ++;
    }
}

os_err_t os_queue_read (os_queue_t * queue, int ms, int opt, void * msg)
{
    os_param_failed(queue == OS_NULL,OS_ERR_PARAM);
    os_param_failed(msg == OS_NULL,OS_ERR_PARAM);

    os_isr_status_t status =  os_sched_isr_disable();

    if(queue->msg_cnt > 0)
    {
        queue_read_msg(queue,opt,msg);

        os_task_t * task = os_event_notify(&queue->write_event);
        /*测试笔记：要满足task ！=0，在进入os_queue_read之前cnt一定是满的，且read和write一定是重合的，进入queue_read_msg之后，
          读了一次，此时read=write + queue->msg_size，如果opt是紧急消息，那么queue_wirte_in里，写入的位置是read - queue->msg_size
          同时也会把read-queue->msg_size，此时read=write，然后进行写入，此时如果再发生一次读取，那么会第一个读到这个紧急消息；
          如果opt是普通消息，那么会写入write的位置，此时如果再发生一次读取，那么会读取read的值，这里不像紧急消息那样会把read的
          值往后移动1格，而是正常读取，所以这个普通消息会被最后读取，与设计目标一致*/
        if(task != OS_NULL)
        {
            task->event_info.err = OS_ERR_OK;
            os_queue_wait_t * wait_write_msg =(os_queue_wait_t *) task->event_info.reason;

            queue_wirte_in(queue,wait_write_msg->opt,wait_write_msg->msg);
        }
        os_sched_isr_enable(status);
        os_sched_run();
        return OS_ERR_OK;
    }else 
    {
        if(ms<0 || os_core.os_sched_in_isr_flag > 0)
        {
            os_dbg("queue_pool  is no cnt  and no delay");
            return OS_ERR_QUEUE_POOL_EMPTY;
        }
        
        /*把要读取的信息的目的，和opt传进task的event_info的reason里面，后面os_queue_write看到有任务在读待读，就直接执行读的操作
        而不是返回到这个任务再读，是把先唤醒任务，再把写入的消息读到保存在reason记录的目的地上，然后再切换任务，等到切换到这个任务，
        就直接从os_queue_read返回*/
        os_queue_wait_t wait_read_msg = {.msg = msg,.opt = opt};
        os_event_wait(&queue->read_event,&wait_read_msg,ms);
        os_sched_isr_enable(status);
        os_sched_run();
        /*任务从这里被切出去*/
        /*---------------------------*/
        /*任务从这里回来，返回值记录是事件成功返回(OS_ERR_OK)，还是超时而返回(OS_ERR_EVENT_TIMEOUT),还是清空队列而返回(OS_ERR_QUEUE_CLAER)*/
        os_task_t * self = os_task_self();
        return self->event_info.err;
    }

    
    os_sched_isr_enable(status);
    return OS_ERR_OK;    
}

/*opt为消息类型*/
os_err_t os_queue_write (os_queue_t * queue, int ms, int opt, void * msg)
{
    os_param_failed(queue == OS_NULL, OS_ERR_PARAM);
    os_param_failed(opt < 0, OS_ERR_PARAM);
    os_param_failed(msg == OS_NULL, OS_ERR_PARAM);

    os_isr_status_t status = os_sched_isr_disable();

    os_task_t *task = os_event_notify(&queue->read_event);
    if (task != OS_NULL) 
    {
        /*已经把queue的read_list的任务给唤醒，在切换到这个任务前就把这个消息给读取了，后面切换到这个任务运行，就返回到
          os_queue_read的最后的return，然后应用层判断返回的err值，判断是成功读到了，还是超时而返回*/
        task->event_info.err = OS_ERR_OK;
        os_queue_wait_t * wait_read_msg = (os_queue_wait_t *)task->event_info.reason;
        /*不经过消息缓存，直接写入目的*/
        memcpy(wait_read_msg->msg,msg,queue->msg_size);

        os_sched_isr_enable(status);
        os_sched_run();
        return OS_ERR_OK;
    }else if(queue->msg_cnt < queue->msg_max_cnt)  /*无任务在等，且消息缓冲区不满，直接写入*/
    {
        queue_wirte_in(queue,opt,msg);
        os_sched_isr_enable(status);
        return OS_ERR_OK;
    }else    /*无任务在等，且消息缓冲区满，根据ms的设置，把任务挂入queue的write_event里面*/                                   
    {
        /*ms<0，不等，直接返回*/
        if(ms < 0 || os_core.os_sched_in_isr_flag > 0)
        {
            os_dbg("queue write buf full");
            os_sched_isr_enable(status);
            return OS_ERR_QUEUE_POOL_FULL;
        }
        /*ms=0，死等，把这个任务移除ready列表，且不移入delay列表，移入write_event的wait_list；
          ms=0，延时等待，把这个任务移除ready列表，同时移入delay列表，移入write_event的wait_list*/

        /*wait_msg的作用是在上面的os_queue_read函数，发现有任务因为缓冲区满而等待写入，用来保存要写入的消息的地址和opt
          然后在os_queue_read函数里面，把这个任务给唤醒，然后直接写入*/  
        os_queue_wait_t wait_write_msg = {.msg = msg,.opt = opt};
        os_event_wait(&queue->write_event,&wait_write_msg,ms);
        os_sched_isr_enable(status);
        os_sched_run();
        /*任务从这里被切出去*/
        /*---------------------------*/
        /*任务从这里回来，返回值记录是事件成功返回(OS_ERR_OK)，还是超时而返回(OS_ERR_EVENT_TIMEOUT),还是清空队列而返回(OS_ERR_QUEUE_CLAER)*/
        os_task_t * self = os_task_self();
        return self->event_info.err;
    }      
}

#endif