#include "timer.h"
#include "io.h"
#include "print.h"
#include "thread.h"
#include "debug.h"
#include "interrupt.h"

#define IRQ_FREQUENCY       100
#define INPUT_FREQUENCY     1193180
#define COUNTERO_VALUE      INPUT_FREQUENCY / IRQ_FREQUENCY
#define COINTERO_PORT       0x40
#define COUNTERO_NO         0
#define COUNTER_MODE        2
#define READ_WRITE_LATCH    3
#define PIT_CONTROL_PORT    0x43        //控制端口

uint32_t ticks;
/*
    把计数器的 计数 写入，设置读写锁、计数器模式
    //1.先设置0x43端口，设置工作模式等
    //2.在端口中设置初始值
*/
//参数说明：
//counter_port端口
//counter_no哪个计数器
//rwl读写锁
//counter_mode:工作模式
//counter_value计数器的初始值
static void frequency_set(uint8_t counter_port,uint8_t counter_no,uint8_t rwl,uint8_t counter_mode,uint16_t counter_value)
{
    outb(PIT_CONTROL_PORT,(uint8_t)(counter_no << 6 | rwl << 4 | counter_mode << 1));
    //下面写入counter_value的低8位和高八位
    outb(counter_port,(uint8_t)counter_value);
    outb(counter_port,(uint8_t)(counter_value >> 8));
}

static void intr_timer_handler(void)
{
    struct task_struct * cur_thread = runing_thread();
    ASSERT(cur_thread->stack_magic == 0x20000319);

    cur_thread->elapsed_ticks++;
    ticks++;
    if(cur_thread->ticks == 0) schedule();
    else cur_thread->ticks--;
}

//初始化计数器
void timer_init()
{
    put_str("timer start\n");
    frequency_set(COINTERO_PORT,COUNTERO_NO,READ_WRITE_LATCH,COUNTER_MODE,COUNTERO_VALUE);
    register_handler(0x20,intr_timer_handler);
    put_str("timer_init done\n");
}