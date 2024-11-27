#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "memory.h"
#include "interrupt.h"
#include "debug.h"
#include "print.h"

#define PG_SIZE 4096

struct task_struct * main_thread;       //主线程的PCB
struct list thread_ready_list;          //就绪队列
struct list thread_all_list;            //所有的任务队列
static struct list_elem* thread_tag;    //用于保存队列中的线程节点


extern void switch_to(struct task_struct * cur,struct task_struct * next);

//获取正在运行线程的pcb
struct task_struct * runing_thread()
{
    uint32_t esp;
    asm("mov %%esp,%0" : "=g"(esp));
    return (struct task_struct *)(esp & 0xfffff000);
}

//由kernel_thread去执行function
void kernel_thread(thread_func * function,void * func_arg)
{
    //保证能被中断打断，重新调度
    intr_enable();
    function(func_arg);
}

//初始化线程栈thread_stack
void thread_create(struct task_struct * pthread,thread_func function,void * func_arg)
{
    pthread->self_ksatck -= sizeof(struct intr_stack);//给中断栈预留空间,线程进入中断的话，通过该栈来保存上下文(还能保存用户进程的初始信息)
    pthread->self_ksatck -= sizeof(struct thread_stack);//在流出线程栈使用的空间
    struct thread_stack * kthread_stack = (struct thread_stack *)pthread->self_ksatck;
    kthread_stack->eip = kernel_thread;
    kthread_stack->function=function;
    kthread_stack->func_arg=func_arg;
    kthread_stack->ebp=kthread_stack->ebx=kthread_stack->esi=kthread_stack->edi=0;
}

//初始化线程基本信息
void init_thread(struct task_struct * pthread,char * name,int prio)
{
    //清理申请的pcb页
    memset(pthread,0,sizeof(*pthread));
    //将线程名写道对应的PCB中的name数组中
    strcpy(pthread->name,name);

    if(pthread == main_thread)  pthread->status=TASK_RUNNING;
    else pthread->status=TASK_READY;

    //线程在0特权级下使用的栈，被初始化PCB的最顶端
    pthread->self_ksatck=(uint32_t*)((uint32_t)pthread + PG_SIZE);
    pthread->priority=prio;
    pthread->ticks=prio;
    pthread->elapsed_ticks=0;
    pthread->pdgir=NULL;
    pthread->stack_magic=0x20000319;
}

//创建一个优先级位prio的线程，线程名叫name
struct task_struct * thread_start(char * name,int prio,thread_func function,void * func_arg)
{
    //thread指向申请的一个页 的 起始地址
    struct task_struct * thread = get_kernel_pages(1);

    init_thread(thread,name,prio);
    thread_create(thread,function,func_arg);

    ASSERT(!elem_find(&thread_ready_list,&thread->general_tag));
    list_append(&thread_ready_list,&thread->general_tag);

    ASSERT(!elem_find(&thread_all_list,&thread->all_list_tag));
    list_append(&thread_all_list,&thread->all_list_tag);

    return thread;
}


//将kernel中的main函数完善成主线程
static void make_main_thread(void)
{
    main_thread = runing_thread();
    init_thread(main_thread,"main",31);

    ASSERT(!elem_find(&thread_all_list,&main_thread->all_list_tag));
    list_append(&thread_all_list,&main_thread->all_list_tag);
}

void schedule()
{
    ASSERT(intr_get_status() == INTR_OFF);

    struct task_struct * cur = runing_thread();
    if(cur->status == TASK_RUNNING)
    {
        ASSERT(!elem_find(&thread_ready_list,&cur->general_tag));
        list_append(&thread_ready_list,&cur->general_tag);
        cur->ticks = cur->priority;
        cur->status = TASK_READY;
    }
    else
    {

    }
    ASSERT(!list_empty(&thread_ready_list));
    thread_tag = NULL;  //thread_tag清空
    thread_tag = list_pop(&thread_ready_list);
    struct task_struct * next = elem2entry(struct task_struct,general_tag,thread_tag);
    next->status = TASK_RUNNING;
    switch_to(cur,next);
}

void thread_init(void)
{
    put_str("thread_init start\n");
    list_init(&thread_ready_list);
    list_init(&thread_all_list);
    make_main_thread();
    put_str("thread_init_done\n");
}