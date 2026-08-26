#ifndef USER_RTOS_OS_MBLOCK_H_
#define USER_RTOS_OS_MBLOCK_H_

#include "os_event.h"
#include "os_def.h"



typedef struct _os_men_lick_t
{
    struct _os_men_lick_t * next;
}os_men_lick_t;

typedef struct _os_mblock_t
{
    os_event_t event;
    
    os_men_lick_t * list;

    void * men_start;

    uint16_t blk_size;

    uint16_t blk_total;  
    uint16_t blk_free;
}os_mblock_t;


#if OS_MBLOCK_EN
os_err_t os_mblock_create_static (os_mblock_t * mblock, void * mem, uint16_t blk_size, uint16_t blk_cnt);
os_err_t os_mblock_uninit (os_mblock_t * mblock);
os_mblock_t * os_mblock_create (uint16_t blk_size, uint16_t blk_cnt);
os_err_t os_mblock_free (os_mblock_t * mblock);

void * os_mblock_wait(os_mblock_t * mblock, int ms, os_err_t * p_err);
os_err_t os_mblock_release (os_mblock_t * mblock, void * mem);

#if OS_MBLOCK_INFO_EN
uint16_t os_mblock_blk_cnt (os_mblock_t * mblock);
uint16_t os_mblock_blk_size (os_mblock_t * mblock);
uint16_t os_mblock_tasks(os_mblock_t * mblock);
#endif
#endif

#endif