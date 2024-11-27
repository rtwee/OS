#include "init.h"
#include "print.h"
#include "interrupt.h"
#include "timer.h"
#include "memory.h"
#include "thread.h"
//负责所有模块的初始化
void init_all()
{
    put_str("I am kernel\n");
    idt_init();     //初始化中断
    mem_init();
    timer_init();
    thread_init();
}