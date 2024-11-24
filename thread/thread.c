#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "memory.h"

#define PG_SIZE 4096

//由kernel_thread去执行function
void kernel_thread(thread_func * function,void * func_arg)
{
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
    pthread->status=TASK_RUNNING;
    pthread->priority=prio;
    //线程在0特权级下使用的栈，被初始化PCB的最顶端
    pthread->self_ksatck=(uint32_t*)((uint32_t)pthread + PG_SIZE);
    pthread->stack_magic=0x20000319;
}

//创建一个优先级位prio的线程，线程名叫name
struct task_struct * thread_start(char * name,int prio,thread_func function,void * func_arg)
{
    //thread指向申请的一个页 的 起始地址
    struct task_struct * thread = get_kernel_pages(1);

    init_thread(thread,function,func_arg);
    thread_create(thread,function,func_arg);
    asm volatile ("movl %0, %%esp; \
    pop %%ebp;pop %%ebx;pop %%edi;pop %%esi; \
    ret"::"g"(thread->self_ksatck):"memory");
    return thread;
}
