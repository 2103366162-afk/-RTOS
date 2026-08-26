/* os_list.h - 通用双向链表模块接口 */
#ifndef USER_RTOS_OS_LIST_H_
#define USER_RTOS_OS_LIST_H_

#include "stdint.h"
#include "os_def.h"
#include "os_dbg.h"
#include "os_lib.h"
#include "os_err.h"

/* 前向声明，链表中可能保存任务对象 */
typedef struct _os_task_t os_task_t;
typedef struct _os_timer_t os_timer_t;
/*
 * 链表节点结构体
 * 该结构体被嵌入到用户自定义的结构体中，用于维护双向链表。
 * pre 和 next 分别指向前驱和后继节点，通过 owner_offset 可以反推出宿主结构体的起始地址。
 */
typedef struct _os_list_item_t
{
    struct _os_list_item_t *pre;    /* 指向前一个节点，若为头节点则为空 */
    struct _os_list_item_t *next;   /* 指向后一个节点，若为尾节点则为空 */
    /*
     * 为什么节点在内存中通常具有固定的 4 字节偏移？
     * 因为 pre 和 next 都是指针，占 4 字节（32位平台），所以嵌入到不同结构体时，
     * 只要结构体从同一地址开始，item 的偏移就固定。owner 字段用于调试，记录宿主结构体地址。
     */
#if OS_LIST_INFO_EN
    union
    {
        void * owner;   /* 指向拥有该节点的主结构体，便于调试观察 */
        os_task_t * task;
        //os_timer_t * timer;
    } owner;
#endif
} os_list_item_t;

/*
 * 链表控制块结构体
 * first 指向链表第一个节点，end 指向最后一个节点。
 * count 记录节点数量，owner_offset 用于计算宿主对象与节点之间的偏移。
 */
 typedef struct _os_list_t
 {
    os_list_item_t * first;  /* 链表头，头节点的 pre 不会被使用，只通过 next 遍历 */
    os_list_item_t * end;    /* 链表尾，尾节点的 next 不会被使用 */
    uint16_t count;          /* 当前链表节点数量 */
    uint16_t owner_offset;   /* 宿主结构体中 os_list_item_t 成员相对于结构体起始地址的偏移 */
 }os_list_t;

/* 返回链表当前节点数量 */
static inline int os_list_count(os_list_t * list)
{
    os_assert(list != OS_NULL);
    return list->count;
}

/* 获取指定节点的下一个节点宿主对象（若存在） */
void * os_list_item_next(os_list_t * list , void *obj);

/* 获取指定节点的上一个节点宿主对象（若存在） */
void * os_list_item_pre (os_list_t * list, void * obj);

/* 初始化链表节点，设置 pre/next 为空，并记录宿主偏移（用于调试） */
void __os_list_item_init(os_list_item_t * item, int offset);

/* 初始化链表控制块，设置空表状态 */
void __os_list_init(os_list_t * list, int offset);

/* 获取链表第一个节点对应的宿主对象 */
void * os_list_first(os_list_t * list);

/* 获取链表最后一个节点（注意实现可能返回节点指针而非宿主对象） */
void * os_list_end(os_list_t * list);

/* 将对象插入链表头部 */
void os_list_insert_first(os_list_t * list,void *obj);

/* 将对象插入链表尾部 */
void os_list_insert_last(os_list_t * list, void *obj);

/* 将 next_obj 插入到 pre_obj 之后；若 pre_obj 为空，则插入头部 */
void os_list_insert_after(os_list_t * list, void * pre_obj,void * next_obj);

/* 移除并返回链表第一个节点对应的宿主对象 */
void *os_list_remove_first(os_list_t * list);

/* 移除指定的对象节点 */
void os_list_remove_item (os_list_t * list, void * obj);

/* 清空链表，并将所有节点的指针置空 */
void os_list_remove_all(os_list_t * list);

/* 打印链表所有节点信息（调试用，需 OS_LIST_INFO_EN 开启） */
void os_list_item_all_show(os_list_t * list,const char * string);

/* 链表完整性校验，仅在开启调试信息时编译 */
#if OS_LIST_INFO_EN
    void os_list_check(os_list_t * list);
#else
#define os_list_check(list)   /* 空实现 */
#endif

/*
 * 初始化节点：计算宿主结构体中 name 成员的偏移
 * 例如：os_list_item_init(&task->list, task_t, list);
 */
#define os_list_item_init(item,type,name)  __os_list_item_init(item, (int) &((type*)0)->name)

/*
 * 初始化链表：记录宿主结构体中 name 成员的偏移
 */
#define os_list_init(list,type,name)  __os_list_init(list, (int) &((type*)0)->name)

/* 从宿主对象指针获取其内部节点指针：obj 加上 owner_offset 纯地址计算，不进行地址访问*/
#define os_obj_to_list_item(obj,list) (os_list_item_t *)((uint8_t *)obj+(list->owner_offset))

/* 从节点指针反推宿主对象指针：item 减去 owner_offset */
#define os_list_item_to_obj(item,list) ((void *)((uint8_t *)item - list->owner_offset))

/*
 * 遍历链表宏
 * 用法示例：
 * os_list_for_each(p, task_t, &list) { ... }
 * 注意该宏会声明局部变量 obj，需在代码块内使用。
 */
#define os_list_for_each(obj, owner_type, list)     \
    /*注意这里的obj是在循环外面定义的，所以退出for，还可以使用obj*/\
    owner_type * obj = OS_NULL;                                          \
    for (obj = (owner_type *)os_list_first(list); obj != OS_NULL; obj = (owner_type *)os_list_item_next(list, obj)) 

#endif /* USER_RTOS_OS_LIST_H_ */