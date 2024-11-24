#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H
#include "stdint.h"

typedef void thread_func(void *);

//进程的几种运行状态
enum task_status
{
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_WAITING,
    TASK_HANGING,
    TASK_DIED
};

//中断发生时维护的栈信息
struct intr_stack
{
    uint32_t vec_no;    //interrupt 中的 push %1压入的中断号
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;

    uint32_t err_code;  //错误码
    void (*eip)(void);  //ip
    uint32_t cs;        //cs段
    uint32_t eflags;
    void * esp;         //如果发生栈的变化
    uint32_t ss;
};

//线程栈
//保存要执行的函数
//在使用switch to时保存现场环境
struct thread_stack
{
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edi;
    uint32_t esi;

    //第一次执行时，eip指向的调用的函数kernel_thread,其他时候eip是指向switch to的返回地址
    void (*eip)(thread_func * func,void * func_arg);

    //unused_ret只为占位置充数返回地址
    void (*unused_retaddr);
    thread_func * function;     //由kernel_thread调用的函数名和参数
    void * func_arg;
};

//进程或者线程的pcb，程序控制块
struct task_struct
{
    uint32_t * self_ksatck; //各个线程都用自己的内核栈
    enum task_status status;
    uint8_t priority;       //栈的优先级
    char name[16];
    uint32_t stack_magic;   //栈的边界标记
};

#endif