#ifndef USER_RTOS_OS_QUEUE_H_
#define USER_RTOS_OS_QUEUE_H_

#include "os_def.h"
#include "os_err.h"
#include <stdint.h>
#include "os_event.h"

#if OS_QUEUE_EN

#define OS_QUEUE_RELESE_UPGENT  2       /*紧急消息*/
#define OS_QUEUE_RELESE_NORMAL  1       /*普通消息*/

typedef struct _os_queue_t
{
    os_event_t read_event;
    os_event_t write_event;

    uint8_t * queue_pool_start;
    uint16_t msg_size;  //一个块的字节数
    uint16_t total_size;  //整个缓存池的字节数

    uint16_t msg_cnt;       //当前队列消息数量
    uint16_t msg_max_cnt;   //最大支持消息的数量

    uint16_t  read;  //读的偏移量
    uint16_t  write; //写的偏移量

}os_queue_t;


os_err_t os_queue_create_static (os_queue_t * queue, void * msg_buf, uint16_t msg_size, uint16_t msg_cnt);
os_err_t os_queue_uninit(os_queue_t * queue);
os_queue_t * os_queue_create (uint16_t msg_size, uint16_t msg_cnt);
os_err_t os_queue_free (os_queue_t * queue);
#if OS_QUEUE_INFO_EN
uint16_t os_queue_msg_cnt (os_queue_t * queue);
uint16_t os_queue_free_cnt (os_queue_t * queue);
uint16_t os_queue_wait_read_task_cnt(os_queue_t * queue);
uint16_t os_queue_wait_write_task_cnt (os_queue_t * queue);
void os_queue_show_status (os_queue_t * queue);
#endif
os_err_t os_queue_clear (os_queue_t * queue);
os_err_t os_queue_read (os_queue_t * queue, int ms, int opt, void * msg);
os_err_t os_queue_write (os_queue_t * queue, int ms, int opt, void * msg);
#endif
#endif