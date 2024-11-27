#include "list.h"
#include "interrupt.h"
#include "stdint.h"
#include "debug.h"

//初始化双向链表
void list_init(struct list* list)
{
    list->head.prev = NULL;
    list->head.next=&list->tail;
    list->tail.prev=&list->head;
    list->tail.next = NULL;
}

//把elem插入到before之前
void list_insert_before(struct list_elem* before,struct list_elem* elem)
{
    enum intr_status old_status = intr_disable();
    
    before->prev->next = elem;
    elem->prev=before->prev;
    elem->next=before;
    before->prev=elem;

    intr_set_status(old_status);
}

//添加队列头，类似push操作
void list_push(struct list* plist,struct list_elem* elem)
{
    list_insert_before(plist->head.next,elem);
}

//添加到队列尾部
void list_append(struct list* plist,struct list_elem* elem)
{
    list_insert_before(&plist->tail,elem);
}

//使pelem从链表中移除
void list_remove(struct list_elem* pelem)
{
    enum intr_status old_status = intr_disable();

    pelem->prev->next = pelem->next;
    pelem->next->prev=pelem->prev;

    intr_set_status(old_status);
}

//将链表第一个元素弹出并返回
struct list_elem* list_pop(struct list* plist)
{
    struct list_elem * elem = plist->head.next;
    list_remove(elem);
    return elem;
}

//从连表中查找元素obj并返回
bool elem_find(struct list* plist,struct list_elem* obj_elem)
{
    struct list_elem * elem = plist->head.next;
    while(elem != &plist->tail)
    {
        if(elem == obj_elem) return 1;
        elem = elem->next;
    }
    return 0;
}


struct list_elem* list_traversal(struct list* plist,function func,int arg)
{
    if(list_empty(plist)) return NULL;
    struct list_elem * elem = plist->head.next;
    while(elem != &plist->tail)
    {
        if(func(elem,arg)) return elem;
        elem = elem->next;
    }
    return NULL;
}

uint32_t list_len(struct list* plist)
{
    struct list_elem * elem = plist->head.next;
    uint32_t length = 0;
    while(elem != &plist->tail)
    {
        length++;
        elem=elem->next;
    }
    return length;
}

bool list_empty(struct list* plist)
{
    return (plist->head.next == &plist->tail ? 1 : 0);
}
