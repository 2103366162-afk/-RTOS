#include "os_sem.h"
#include "os_event.h"
#include "os_sched.h"
#include "os_dbg.h"

#if OS_SEM_EN

#if !OS_SEM_DBG_PRINT_EN
#undef os_dbg
#define os_dbg(fmt, ...) do {}while (0)
#endif

extern os_core_t os_core;

static os_err_t sem_init(os_sem_t * sem, int init_cnt, int max_cnt,int flag)
{
    os_err_t err = os_event_init(&sem->event,OS_EVENT_TYPE_SEM,flag);
    if(err<0)
    {
        os_dbg("sem event init faied ");
        return  err;
    }

    if( max_cnt>0 && init_cnt > max_cnt)
    {
        os_dbg("waring:init_cnt > max_cnt");
        init_cnt = max_cnt;
    }

    sem->curr_cnt = init_cnt;
    sem->max_cnt = max_cnt;
    return OS_ERR_OK;
}

os_err_t os_sem_create_static (os_sem_t * sem, int init_cnt, int max_cnt)
{
    os_param_failed(sem == OS_NULL,OS_ERR_PARAM);
    os_param_failed(init_cnt < 0,OS_ERR_PARAM);
    os_param_failed(max_cnt < 0 || max_cnt > 255, OS_ERR_PARAM); 

    return sem_init(sem,init_cnt,max_cnt,0);
}

os_err_t os_sem_uninit(os_sem_t * sem)
{
    os_param_failed(sem == 0,OS_ERR_PARAM);

    os_isr_status_t status =  os_sched_isr_disable();
    int cnt = os_event_wait_cnt(&sem->event);
    os_event_uninit(&sem->event);
    if(cnt > 0)
    {
        os_sched_run();
    }
    os_sched_isr_enable(status);

    return OS_ERR_OK;
}

os_sem_t * os_sem_create (int init_cnt, int max_cnt)
{
    os_param_failed(init_cnt < 0,OS_NULL);
    os_param_failed(max_cnt > 255 || max_cnt < 0,OS_NULL);
    /*����sem�ṹ��*/
    os_sem_t * sem = os_mem_malloc(sizeof(os_sem_t));
    if(sem == OS_NULL)
    {
        os_dbg("error:sem mem_malloc failed");
        return  OS_NULL;
    }

    os_err_t err = sem_init(sem, init_cnt,max_cnt ,OS_FLAG_MEM_HEAP);
    if(err<0)
    {
        os_dbg("error:sem init failed");
        os_men_free(sem);
        return OS_NULL;
    }
    return sem;
}

os_err_t os_sem_free (os_sem_t * sem)
{
    os_param_failed(sem == OS_NULL,OS_ERR_PARAM);

    os_err_t err = os_sem_uninit(sem);
    if(err<0)
    {
        os_dbg("sem uninit failed");
        return err;
    }

    if(sem->event.flag & OS_FLAG_MEM_HEAP)
    {
        os_men_free(sem);
    }

    return  OS_ERR_OK;
}

os_err_t os_sem_take (os_sem_t * sem, int ms)
{
    os_param_failed(sem == OS_NULL, OS_ERR_PARAM);
    os_param_failed(sem->curr_cnt< 0,OS_ERR_PARAM);
    os_param_failed(sem->curr_cnt > sem->max_cnt,OS_ERR_PARAM);

    os_isr_status_t status = os_sched_isr_disable();
    os_task_t *self = os_task_self();

    /* ��ֹ���ж��е�������汾 */
    if (os_core.os_sched_in_isr_flag > 0) {
        os_sched_isr_enable(status);
        return OS_ERR_ISR;  
    }

    if (sem->curr_cnt > 0) {
        /* �п����ź������ɹ���ȡ */
        sem->curr_cnt--;
        os_sched_isr_enable(status);
        return OS_ERR_OK;
    }

    /* curr_cnt == 0���ź��������� */
    if (ms < 0) {
        /* ��������ȡʧ�� */
        os_sched_isr_enable(status);
        return OS_ERR_SEM_CNT_ZERO;
    }

    /* �����ȴ� */
    os_event_wait(&sem->event, OS_NULL, ms);
    os_sched_isr_enable(status);
    os_sched_run();   
    return self->event_info.err;
}

os_err_t os_sem_release (os_sem_t * sem)
{
    os_param_failed(sem == OS_NULL, OS_ERR_PARAM);

    os_isr_status_t status = os_sched_isr_disable();

    /* �ж��н�ֹ���ã���ʹ�� os_sem_release_from_isr */
    if (os_core.os_sched_in_isr_flag > 0) 
    {
        os_dbg("sem_release in isr");
        os_sched_isr_enable(status);
        return OS_ERR_SEM_CNT_ZERO;
    }

    int cnt = os_event_wait_cnt(&sem->event);
    if (cnt > 0) 
    {
        os_task_t *task = os_event_notify(&sem->event);
        if (task != OS_NULL) 
        {
            task->event_info.err = OS_ERR_OK;
            task->event_info.reason = OS_NULL;
        }
        os_sched_isr_enable(status);
        os_sched_run();  
        return OS_ERR_OK;
    }

    /* ������ȴ������Ӽ��� */
    if (sem->curr_cnt < sem->max_cnt) 
    {
        sem->curr_cnt++;
        os_sched_isr_enable(status);
        return OS_ERR_OK;
    }

    /* ���� */
    os_dbg("sem_release cnt over max_cnt");
    os_sched_isr_enable(status);

    return OS_ERR_SEM_CNT_OUT;
}

/*********************************************************************
 * @fn      os_sem_take_from_isr
 *
 * @brief   �ж��г��Ի�ȡ�ź�������������
 *          ����ź������� >0�����һ���� OK��
 *          �����������ش��󣬲���ȴ���
 *          �������ж��е��û������� os_sem_take��
 *
 * @return  OS_ERR_OK    ��ȡ�ɹ�
 *          OS_ERR_SEM_CNT_ZERO  ��ǰ����Ϊ0����ȡʧ��
 */
os_err_t os_sem_take_from_isr (os_sem_t * sem)
{
    os_param_failed(sem == OS_NULL, OS_ERR_PARAM);

    os_isr_status_t status = os_sched_isr_disable();

    if (sem->curr_cnt > 0) {
        sem->curr_cnt--;
        os_sched_isr_enable(status);
        return OS_ERR_OK;
    }

    os_sched_isr_enable(status);
    return OS_ERR_SEM_CNT_ZERO;
}

/*********************************************************************
 * @fn      os_sem_release_from_isr
 *
 * @brief   �ж����ͷ��ź�������������
 *          ����������ڵȴ�������һ�����񣨵��������л����л���
 *          os_isr_leave() ���������������+1��
 *
 * @return  OS_ERR_OK              �ɹ�
 *          OS_ERR_SEM_CNT_OUT     �����������޷��ͷ�
 */
os_err_t os_sem_release_from_isr (os_sem_t * sem)
{
    os_param_failed(sem == OS_NULL, OS_ERR_PARAM);

    os_isr_status_t status = os_sched_isr_disable();

    int cnt = os_event_wait_cnt(&sem->event);
    if (cnt > 0) {
        /* �������ڵȴ������Ѷ������� */
        os_task_t *task = os_event_notify(&sem->event);
        if (task != OS_NULL) {
            task->event_info.err = OS_ERR_OK;
            task->event_info.reason = OS_NULL;
        }
        /* ע�⣺���ﲻ�ܵ��� os_sched_run()����Ϊ ISR �л�ͳһ�� os_isr_leave() ���� */
        os_sched_isr_enable(status);
        return OS_ERR_OK;
    }

    /* ������ȴ������Ӽ��� */
    if (sem->curr_cnt < sem->max_cnt) {
        sem->curr_cnt++;
        os_sched_isr_enable(status);
        return OS_ERR_OK;
    }

    os_sched_isr_enable(status);
    os_dbg("sem_release_from_isr cnt over max_cnt");
    return OS_ERR_SEM_CNT_OUT;
}

#if OS_SEM_INFO_EN
int os_sem_cnt (os_sem_t * sem)
{
    os_param_failed(sem == OS_NULL,OS_ERR_PARAM);
    
    os_isr_status_t status =  os_sched_isr_disable();
    uint16_t cnt = sem->curr_cnt;
    os_sched_isr_enable(status);
    return cnt;
}

int os_sem_max (os_sem_t * sem)
{
    os_param_failed(sem == OS_NULL,OS_ERR_PARAM);
    
    os_isr_status_t status =  os_sched_isr_disable();
    uint16_t cnt = sem->max_cnt;
    os_sched_isr_enable(status);
    return cnt;
}

int os_sem_wait_tasks_cnt (os_sem_t * sem)
{
    os_param_failed(sem == OS_NULL,OS_ERR_PARAM);

    os_isr_status_t status =  os_sched_isr_disable();
    uint16_t task_cnt = os_event_wait_cnt(&sem->event);
    os_sched_isr_enable(status);
    return task_cnt;
}

#endif

#endif