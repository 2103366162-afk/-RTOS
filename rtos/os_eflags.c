#include "os_def.h"
#include "os_eflags.h"
#include "os_sched.h"
#include "os_men.h"
#include "os_event.h"
#include  <stdbool.h>

#if OS_EFLAGS_EN

/*��������Ҫ�ȴ���λ����Ϣ*/
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

    /*����os_eflags_t�ṹ��*/
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

    /*�ȼ�����е������Ƿ�����*/ 
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


    /*���������㣬��ǰ������Ҫ�ȴ�*/
    os_eflags_wait_t task_flags_info = {.mask = mask,.result = 0,.type = type};
    os_event_wait(&eflags->event,&task_flags_info,ms);
    os_sched_isr_enable(status);
    os_sched_run();
    /*��������ﱻ�г�ȥ*/
    /*---------------------------*/
    /*������������������ֵ��¼���¼��ɹ�����(OS_ERR_OK)�����ǳ�ʱ������(OS_ERR_EVENT_TIMEOUT)*/
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

        /*��ȡtask��������Щλ��1��*/
        os_flags_t set_flags = eflags->flags & task_flags_info->mask;
        int check_all = task_flags_info->type & OS_EFLAGS_ALL;
        /*��λ���*/
        if(task_flags_info->type & OS_EFLAGS_SET)
        {
            /*ȫ������1����� �� ����1λ����1�����*/
            if((check_all && (task_flags_info->mask == set_flags))  || (!check_all && (set_flags != 0)))
            {
                /*�������˳�ʱ�����ر�־���Ѷ�Ӧ��λ��0(1->0),�����type��task_flags_info->type��������os_eflags_wait_bits���õģ�
                ��Ϊ�������һ��������os_eflags_wait_bits������β�� β����ֱ��return�����Ըɴ��ڻ���ǰ����ɾ��*/
                if(task_flags_info->type & OS_EFLAGS_EXIT_CLEAR)
                {
                    temp_flags &= ~set_flags;
                }

                os_event_notify_task(&eflags->event,first_task,OS_NULL,OS_ERR_OK);

                task_flags_info->result = set_flags;
                need_sched= true;
                continue;
            }
            /*������*/
        }else if (task_flags_info->type & OS_EFLAGS_CLEAR)
        {
            os_flags_t clear_flags =  ~(eflags->flags & task_flags_info->mask);
            if((check_all && (task_flags_info->mask  == clear_flags)) || (!check_all && (clear_flags != 0 )))
            {
                /*�������˳�ʱ�����ر�־���Ѷ�Ӧ��λ��1(0->1)*/
                if(task_flags_info->type & OS_EFLAGS_EXIT_CLEAR)
                {
                    temp_flags |= clear_flags; 
                }
            
                os_event_notify_task(&eflags->event,first_task,OS_NULL,OS_ERR_OK);

                task_flags_info->result = clear_flags;
                need_sched= true;
                continue;
            }
        }
        /*������������������, first_task��������������������list��β��*/
        if (task_cnt > 1) 
        {
            os_event_remove_task(&eflags->event, first_task);   // �Ƴ�������˱�־
            // ���������¼��ȴ���־�� event ָ��
            first_task->task_flags |= OS_TASK_EVENT_WAIT;
            first_task->event_info.event = &eflags->event;
            // ���뵽�ȴ��б�β��
            os_list_insert_last(&eflags->event.wait_list, first_task);
        }
    }
    eflags->flags = temp_flags;
    os_sched_isr_enable(status);
    
    if(need_sched)
    {
        /*��Ȼǰ�滽�������ʱ������reason��ֵ��0����������ֻ�ı�task�ṹ���reasonֵ��ȴ���Ǹı�task_flags_info�ṹ���ֵ
          ���淵�ص����ѵ�����ʱ����ͨ��reasonȥ����task_flags_info����Ϣ������������������棬�Ѿ�ͨ��ָ�����޸���task_flags_info
          ��ֵ������ȥ���ֽڶ�ȡtask_flags_info�Ľ������*/
        os_sched_run();
        /*��������ﱻ�г�ȥ*/
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