#ifndef USER_RTOS_OS_MEN_H_
#define USER_RTOS_OS_MEN_H_

#include "os_err.h"
#include "os_list.h"

#define MEN_HIGH_ALIGED(v,align)    (((uint32_t)(v) + ((align) -1)) & ~((uint32_t)(align) - 1))
#define MEN_LOW_ALIGED(v,align)     (((uint32_t)(v)) & ~((uint32_t)(align) - 1))

#if OS_MEN_EN
#define  OS_FLAG_MEM_HEAP  (1<<0)

typedef struct _os_men_item_t
{
    os_list_item_t item;
    struct
    {
        uint32_t used : 1 ;
        uint32_t data_size : 31;
    };
}os_men_item_t;

typedef struct _os_men_t
{
    os_list_t list;
    
    void * heap_statck;
    int heap_size;
    int malloc_count;
}os_men_t;

os_err_t os_men_init(void);
void * os_mem_malloc(int size);
os_err_t os_men_free(void * men);
int os_get_free_size(void);
#endif

#endif 