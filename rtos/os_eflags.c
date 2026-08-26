#include "os_def.h"
#include "os_eflags.h"
#include "os_sched.h"
#include "os_men.h"
#include "os_event.h"
#include  <stdbool.h>

#if OS_EFLAGS_EN

/*保存任务要等待的位的信息*/
typedef struct _os_eflags_wait_t
{
    int type;
    os_flags_t mask;
    os_flags_t result;
}os_eflags_wait_t;

static os_err_t eflags_init(os_eflags_t * eflags, os_flags_t init_flags,int flags)
{
    os_err_t err = os_event_init(&eflags->event,OS_EVENT_TYPE_EFLAGS,flags);
    if(err<0)
    {
        os_dbg("eflags event init faied ");
        return  err;
    }
    eflags->flags = init_flags;
    return OS_ERR_OK;
}

os_err_t os_eflags_create_static (os_eflags_t * eflags, os_flags_t init_flags)
{
    os_param_failed(eflags == OS_NULL,OS_ERR_PARAM);
    os_param_failed(init_flags < 0,OS_ERR_PARAM);

    return eflags_init(eflags,init_flags,0);
}

os_err_t os_eflags_uninit(os_eflags_t * eflags)
{
    os_param_failed(eflags == OS_NULL,OS_ERR_PARAM);

    os_isr_status_t status =  os_sched_isr_disable();
    int cnt = os_event_wait_cnt(&eflags->event);
    os_event_uninit(&eflags->event);
    if(cnt > 0)
    {
        os_sched_run();
    }
    os_sched_isr_enable(status);

    return OS_ERR_OK;
}

os_eflags_t * os_eflags_create (os_flags_t init_flags)
{
    os_param_failed(init_flags < 0,OS_NULL);

    /*分配os_eflags_t结构体*/
    os_eflags_t * eflags = os_mem_malloc(sizeof(os_eflags_t));
    if(eflags == OS_NULL)
    {
        os_dbg("error:eflags mem_malloc failed");
        return  OS_NULL;
    }

    os_err_t err = eflags_init(eflags, init_flags ,OS_FLAG_MEM_HEAP);
    if(err<0)
    {
        os_dbg("error:eflags init failed");
        os_men_free(eflags);
        return OS_NULL;
    }
    return eflags;
}

os_err_t os_eflags_free (os_eflags_t * eflags)
{
    os_err_t err = os_eflags_uninit(eflags);
    if(err<0)
    {
        os_dbg("eflags uninit failed");
        return err;
    }

    if(eflags->event.flag & OS_FLAG_MEM_HEAP)
    {
        os_men_free(eflags);
    }

    return  OS_ERR_OK;
}

os_flags_t os_eflags_wait_bits (os_eflags_t * eflags, int ms, int type, os_flags_t mask,  os_err_t * p_err)
{
    os_param_failed(eflags == OS_NULL,OS_ERR_PARAM);
    os_param_failed(mask == 0 ,OS_ERR_PARAM);

    os_isr_status_t status =  os_sched_isr_disable();

    /*先检查已有的条件是否满足*/ 
    int check_all = type & OS_EFLAGS_ALL;
    if(type & OS_EFLAGS_SET)
    {
        os_flags_t set_flags = eflags->flags & mask;
        if((check_all && (set_flags == mask))  || (!check_all && (set_flags != 0)))
        {
            if(type & OS_EFLAGS_EXIT_CLEAR)
            {
                eflags->flags &= ~set_flags;
            }
            os_sched_isr_enable(status);

            if(p_err != OS_NULL)
            {
                *p_err = OS_ERR_OK;
            }
            return set_flags;
        }
    }else if(type & OS_EFLAGS_CLEAR)
    {
        os_flags_t clear_flags = ~(eflags->flags & mask);
        if((check_all && (mask  == clear_flags)) || (!check_all && (clear_flags != 0 )))
        {
            if(type & OS_EFLAGS_EXIT_CLEAR)
            {
                eflags->flags |= clear_flags;
            }

            os_sched_isr_enable(status);

            if(p_err != OS_NULL)
            {
                *p_err = OS_ERR_OK;
            }
            return clear_flags;
        }
    }


    /*条件不满足，当前任务需要等待*/
    os_eflags_wait_t task_flags_info = {.mask = mask,.result = 0,.type = type};
    os_event_wait(&eflags->event,&task_flags_info,ms);
    os_sched_isr_enable(status);
    os_sched_run();
    /*任务从这里被切出去*/
    /*---------------------------*/
    /*任务从这里回来，返回值记录是事件成功返回(OS_ERR_OK)，还是超时而返回(OS_ERR_EVENT_TIMEOUT)*/
    os_task_t *self = os_task_self();
    if(p_err != OS_NULL)
    {
        *p_err = self->event_info.err;
    }
    
    return self->event_info.err == OS_ERR_OK ? task_flags_info.result : 0;
}

os_err_t os_eflags_set_bits (os_eflags_t * eflags, int type, os_flags_t mask)
{
    os_param_failed(eflags == OS_NULL,OS_ERR_PARAM);
    os_param_failed(mask == 0 ,OS_ERR_PARAM);

    os_isr_status_t status =  os_sched_isr_disable();

    if(type & OS_EFLAGS_SET)
    {
        eflags->flags |= mask;
    }else if(type & OS_EFLAGS_CLEAR)
    {
        eflags->flags &= ~mask;
    }    

    os_flags_t temp_flags = eflags->flags;
    bool need_sched= false;
    int task_cnt = os_event_wait_cnt(&eflags->event);
    for(int i = 0;i<task_cnt;i++)
    {
        os_task_t * first_task = os_list_first(&eflags->event.wait_list);
        os_eflags_wait_t * task_flags_info = (os_eflags_wait_t *)first_task->event_info.reason;

        /*获取task的掩码哪些位置1了*/
        os_flags_t set_flags = eflags->flags & task_flags_info->mask;
        int check_all = task_flags_info->type & OS_EFLAGS_ALL;
        /*置位检查*/
        if(task_flags_info->type & OS_EFLAGS_SET)
        {
            /*全部都置1的情况 和 任意1位都置1的情况*/
            if((check_all && (task_flags_info->mask == set_flags))  || (!check_all && (set_flags != 0)))
            {
                /*设置了退出时清除相关标志，把对应的位置0(1->0),这里的type，task_flags_info->type是在任务os_eflags_wait_bits设置的，
                因为唤醒完第一个运行是os_eflags_wait_bits函数的尾部 尾部是直接return，所以干脆在唤醒前进行删除*/
                if(task_flags_info->type & OS_EFLAGS_EXIT_CLEAR)
                {
                    temp_flags &= ~set_flags;
                }

                os_event_notify_task(&eflags->event,first_task,OS_NULL,OS_ERR_OK);

                task_flags_info->result = set_flags;
                need_sched= true;
                continue;
            }
            /*清除检查*/
        }else if (task_flags_info->type & OS_EFLAGS_CLEAR)
        {
            os_flags_t clear_flags =  ~(eflags->flags & task_flags_info->mask);
            if((check_all && (task_flags_info->mask  == clear_flags)) || (!check_all && (clear_flags != 0 )))
            {
                /*设置了退出时清除相关标志，把对应的位置1(0->1)*/
                if(task_flags_info->type & OS_EFLAGS_EXIT_CLEAR)
                {
                    temp_flags |= clear_flags; 
                }
            
                os_event_notify_task(&eflags->event,first_task,OS_NULL,OS_ERR_OK);

                task_flags_info->result = set_flags;
                need_sched= true;
                continue;
            }
        }
        /*两个或两个任务以上, first_task不满足条件，把它移入list的尾部*/
        if (task_cnt > 1) 
        {
            os_event_remove_task(&eflags->event, first_task);   // 移出，清除了标志
            // 重新设置事件等待标志和 event 指针
            first_task->task_flags |= OS_TASK_EVENT_WAIT;
            first_task->event_info.event = &eflags->event;
            // 插入到等待列表尾部
            os_list_insert_last(&eflags->event.wait_list, first_task);
        }
    }
    eflags->flags = temp_flags;
    os_sched_isr_enable(status);
    
    if(need_sched)
    {
        /*虽然前面唤醒任务的时候，设置reason的值是0，但是这里只改变task结构体的reason值，却不是改变task_flags_info结构体的值
          后面返回到唤醒的任务时，不通过reason去访问task_flags_info的信息，而是在这个函数里面，已经通过指针间接修改了task_flags_info
          的值，返回去，字节读取task_flags_info的结果就行*/
        os_sched_run();
        /*任务从这里被切出去*/
        /*---------------------------*/
    }
    return OS_ERR_OK;
}

#if OS_EFLAGS_INFO_EN
os_flags_t os_eflags_get_flags (os_eflags_t * eflags, os_err_t * err)
{
    os_param_failed_exec(eflags == OS_NULL,0,if(err) *err = OS_ERR_PARAM);
    
    os_isr_status_t status =  os_sched_isr_disable();
    os_flags_t flags = eflags->flags;
    os_sched_isr_enable(status);
    return flags;
}

int os_eflags_wait_task_cnt (os_eflags_t * eflags)
{
    os_param_failed(eflags == OS_NULL,OS_ERR_PARAM);

    os_isr_status_t status =  os_sched_isr_disable();
    uint16_t task_cnt = os_event_wait_cnt(&eflags->event);
    os_sched_isr_enable(status);
    return task_cnt;
}

#endif
#endif