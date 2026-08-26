/* os_list.c - 通用双向链表模块实现 */

#include "os_def.h"
#include "os_list.h"
#include "os_err.h"
#include "os_dbg.h"
#include "os_lib.h"

#if OS_LIST_INFO_EN
/*
 * 链表完整性校验函数
 * 检查链表的 first/end/count 一致性以及节点间的双向链接关系。
 * 仅在 OS_LIST_INFO_EN 非零时编译，用于调试。
 */
void os_list_check (os_list_t * list) {
    os_assert(list != OS_NULL);

    os_list_item_t * first = list->first;
    os_list_item_t * end = list->end;

    /* 检查 list 的 first、end、count 合法性 */
    os_assert(list->count >= 0);
    if (list->count == 0) {
        /* 空表时，first 和 end 必须都为 NULL */
        os_assert(!list->first && !list->end);
    } else {
        /* 非空表时，first 和 end 都不能为 NULL */
        os_assert(list->first && list->end);
        if (list->count == 1) {
            /* 只有一个节点时，first 和 end 必须指向同一节点 */
            os_assert(list->first == list->end);
        } else {
            /* 多于一个节点时，first 和 end 不能相同 */
            os_assert(list->first != list->end);
        }
    }

    /* 遍历链表，检查前后链接关系 */
    int count = 0;
    for (os_list_item_t * curr = list->first; curr; curr = curr->next) {
        /* 头节点的 pre 必须为空，非头节点的 pre 不能为空 */
        if (curr == first) {
            os_assert(curr->pre == OS_NULL);
        } else {
            os_assert(curr->pre != OS_NULL);
        }

        /* 尾节点的 next 必须为空，非尾节点的 next 不能为空 */
        if (curr == end) {
            os_assert(curr->next == OS_NULL);
        } else {
            os_assert(curr->next != OS_NULL);
        }

        /* 检查相邻节点指针的对称性 */
        if (curr->next) {
            os_assert(curr->next->pre == curr);
        }
        if (curr->pre) {
            os_assert(curr->pre->next == curr);
        }

        /* TODO: 将来可增加更复杂的结构一致性检查 */
        count++;
    }

    /* 遍历计数必须与链表的 count 一致 */
    os_assert(count == list->count);
}
#endif /* OS_LIST_INFO_EN */

/*
 * 获取指定对象之后的下一个对象
 * list  : 所属链表
 * obj   : 当前对象
 * 返回  : 下一个对象，若已到尾部则返回 OS_NULL
 */
void * os_list_item_next(os_list_t * list , void *obj)
{
    os_assert(list != OS_NULL);
    os_assert(obj != OS_NULL);
    /* 根据宿主对象计算节点指针 */
    os_list_item_t *item  = os_obj_to_list_item(obj,list);
    if(item->next != OS_NULL)
    {
        /* 存在后继节点，反推宿主对象并返回 */
        return os_list_item_to_obj(item->next,list);
    }
    return OS_NULL;
}

/*
 * 获取指定节点的上一个节点宿主对象
 * list  : 所属链表
 * obj   : 当前对象
 * 返回  :  上一个对象，若已到 头部则返回 OS_NULL
 */
void * os_list_item_pre (os_list_t * list, void * obj) {
    os_assert(list != OS_NULL);
    os_assert(obj != OS_NULL);

    os_list_item_t * item = os_obj_to_list_item(obj, list);
    if (item->pre) {
        return os_list_item_to_obj(item->pre, list);
    }

    return OS_NULL;
}

/*
 * 初始化链表节点
 * item   : 节点指针
 * offset : 宿主结构体中该节点的偏移量（用于调试信息记录 owner）
 */
void __os_list_item_init(os_list_item_t * item, int offset)
{
    item->pre = OS_NULL;
    item->next = OS_NULL;
#if OS_LIST_INFO_EN
    /* 记录宿主结构体起始地址：由当前节点地址减去偏移 */
    item->owner.owner = (void *)((int)item - offset); 
#endif
}

/*
 * 初始化链表控制块
 * list   : 链表指针
 * offset : 宿主结构体中节点成员的偏移量，保存到链表控制块中
 */
void __os_list_init(os_list_t * list, int offset)
{
    list->first = OS_NULL;
    list->end = OS_NULL;
    list->count = 0;
    list->owner_offset = (uint16_t)offset;
}

/*
 * 获取链表第一个节点对应的宿主对象
 * list : 链表指针
 * 返回 : 第一个对象，若链表为空则返回 OS_NULL
 */
void * os_list_first(os_list_t * list)
{
    os_assert(list != OS_NULL);
    
    if(list->first != OS_NULL)
    {
        /* 由首个节点反推宿主对象 */
        return os_list_item_to_obj(list->first,list);
    }
    return OS_NULL;
}

/*
 * 获取链表最后一个节点
 * 注意：此实现返回的是节点指针，而非宿主对象指针。
 * 如果要获取宿主对象，应使用 os_list_item_to_obj 手动转换。
 * list : 链表指针
 * 返回 : 尾节点指针，或 OS_NULL
 */
void * os_list_end(os_list_t * list)
{
    os_assert(list != OS_NULL);

    if(list->end != OS_NULL)
    {
        /* 此处实现似乎有误：list->end 已是节点指针，不应再调用 os_obj_to_list_item */
        /* 根据宏定义，os_obj_to_list_item 会误加上 owner_offset，导致错误地址 */
        /* 正确做法应为 os_list_item_to_obj(list->end,list) */
        return os_list_item_to_obj(list->end,list);
    }
    return OS_NULL;
}

/*
 * 将对象插入链表头部
 * list : 链表指针
 * obj  : 待插入宿主对象
 */
void os_list_insert_first(os_list_t * list,void *obj)
{
    os_assert(list != OS_NULL);
    os_assert(obj != OS_NULL);

    /* 获取对象内嵌的节点指针 */
    os_list_item_t *item  = os_obj_to_list_item(obj,list);
    /* 待插入节点必须不在任何链表中 */
    os_assert((item ->pre == OS_NULL) && (item->next == OS_NULL));

    /* 修改新节点自身的指针 */
    item ->pre = OS_NULL;
    item ->next = list->first;

    /* 如果链表非空，更新原头节点的前驱指针 */
    if(list->first != OS_NULL && list->end != OS_NULL)
    {
        list->first->pre = item;
        list->first = item;
    }else
    {
        /* 空表情况，first 和 end 都指向新节点 */
        list->first = item;
        list->end = item;
    }
    list->count++;
    #if OS_LIST_INFO_EN
        os_list_check(list);   /* 调试时校验链表完整性 */
    #endif
}

/*
 * 移除链表第一个节点
 * list : 链表指针
 * 返回 : 被移除的宿主对象，若链表为空则返回 OS_NULL
 */
void * os_list_remove_first(os_list_t * list)
{
    os_assert(list!=OS_NULL);
    if(list->count > 0)
    {
        os_list_item_t *item = list->first;
        
        /* 如果存在后继节点，更新其后继的前驱指针 */
        if(item->next)
        {
            item->next->pre = OS_NULL;
        }
        /* first 指向下一个节点 */
        list->first = item->next;

        /* 如果移除的是唯一个节点，则 end 也要置空 */
        if(list->end == item)
        {
            list->end = OS_NULL;
        }

        list->count--;
        /* 清空被移除节点的指针 */
        item->next = OS_NULL;
        item->pre = OS_NULL;

        #if OS_LIST_INFO_EN
        os_list_check(list);
        #endif

        /* 返回宿主对象 */
        return os_list_item_to_obj(item,list);
    }
    return OS_NULL;
}

/*
 * 将对象插入链表尾部
 * list : 链表指针
 * obj  : 待插入宿主对象
 */
void os_list_insert_last(os_list_t * list, void *obj)
{
    os_assert(list != OS_NULL);
    os_assert(obj != OS_NULL);

    os_list_item_t *item  = os_obj_to_list_item(obj,list);
    os_assert((item ->pre == OS_NULL) && (item->next == OS_NULL));

    /* 新节点前驱指向当前尾节点，后继为空 */
    item ->pre = list->end;
    item ->next = OS_NULL;
    
    /* 如果链表非空，更新原尾节点的后继指针 */
    if(list->first != OS_NULL && list->end != OS_NULL)
    {
        list->end->next = item;
        list->end = item;
    }else  /* 空表 */
    {
        list->first = item;
        list->end = item;
    }
    list->count++;
    #if OS_LIST_INFO_EN
        os_list_check(list);
    #endif
}

/*
 * 将 next_obj 插入到 pre_obj 之后
 * list     : 链表指针
 * pre_obj  : 链表中的既有对象，若为 OS_NULL 则插入头部
 * next_obj : 待插入的新对象
 */
void os_list_insert_after(os_list_t * list, void * pre_obj,void * next_obj)
{
    os_assert(list != OS_NULL);
    os_assert(next_obj != OS_NULL);

    /* pre_obj 为空时直接插到头部 */
    if(pre_obj == OS_NULL)
    {
        os_list_insert_first(list,next_obj);
        return;
    }
    os_list_item_t * pre_item = os_obj_to_list_item(pre_obj,list);
    os_list_item_t * next_item = os_obj_to_list_item(next_obj,list);
    os_assert((next_item->next == OS_NULL) && (next_item->pre == OS_NULL));

    /* 建立新节点的前后链接 */
    next_item->pre = pre_item;
    next_item->next = pre_item->next;

    /* 若 pre_item 有后继，则更新后继的前驱指针 */
    if(pre_item->next != OS_NULL)
    {
        pre_item->next->pre = next_item;
    }
    pre_item->next = next_item;

    /* 如果 pre_item 是尾节点，则更新 end */
    if(list->end == pre_item)
    {
        list->end = next_item;
    }
    list->count++;

    #if OS_LIST_INFO_EN
    os_list_check(list);
    #endif
}

/*
 * 从链表中移除指定对象节点
 * list : 链表指针
 * obj  : 要移除的宿主对象
 */
void os_list_remove_item (os_list_t * list, void * obj) 
{
    os_assert(list !=OS_NULL);
    os_assert(obj != OS_NULL);   
    os_assert(list->count !=0);   /* 链表不能为空 */

    os_list_item_t *item = os_obj_to_list_item(obj,list);

    /* 如果 item 不是头节点，则前驱的后继指向 item 的后继 */
    if(item->pre != OS_NULL)
    {
        item->pre->next = item->next;
    }
    
    /* 如果 item 不是尾节点，则后继的前驱指向 item 的前驱 */
    if(item->next != OS_NULL)
    {
        item->next->pre = item->pre; 
    }

    /* 如果 item 是头节点，则更新 first */
    if(list->first == item)
    {
        list->first = item->next;
    }

    /* 如果 item 是尾节点，则更新 end */
    if(list->end == item)
    {
        list->end = item->pre;
    }
    /* 清空被移除节点的指针 */
    item->next = item->pre = OS_NULL;
    list->count--;

    #if OS_LIST_INFO_EN
    os_list_check(list);
    #endif
}

/*
 * 清空链表所有节点，并将每个节点的链接指针置空
 * list : 链表指针
 */
void os_list_remove_all(os_list_t * list)
{
    os_assert(list != OS_NULL);

    os_list_item_t *curr_item = list ->first;
    os_list_item_t *next_item = list->first;

    /* 遍历所有节点，清空其链接指针 */
    while(next_item != OS_NULL)
    {
        curr_item = next_item;  
        next_item = curr_item->next;
        
        curr_item->pre = OS_NULL;
        curr_item->next = OS_NULL;
    }
    /* 重置链表控制块 */
    list->first = OS_NULL;
    list->end  = OS_NULL;
    list->count =0;

    #if OS_LIST_INFO_EN
    os_list_check(list);
    #endif
}