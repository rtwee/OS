#include "interrupt.h"
#include "stdint.h"
#include "global.h"
#include "print.h"

#include "io.h"

#define IDT_DESC_CNT 0x21       //目前总共支持的中断数量

#define PIC_M_CTRL 0x20         //主片控制端口 
#define PIC_M_DATA 0x21         //主片数据端口
#define PIC_S_CTRL 0xa0         //从片控制端口
#define PIC_S_DATA 0xa1         //从片数据端口

#define EFLAGS_IF   0x00000200  //eflags寄存器中的if位为1,要进行中断屏蔽的时候就是设置这个寄存器
#define GET_EFLAGS(EFLAGS_VAR) asm volatile("pushfl; popl %0" : "=g"(EFLAGS_VAR))   //将flags寄存器压栈，在弹出到变量中

//中断门描述符结构体8字节，对应按顺序从低位到高位
struct gate_desc
{
    uint16_t func_offset_low_word;
    uint16_t selector;
    uint8_t dcount;
    uint8_t attribute;
    uint16_t func_offset_high_word;
};

//静态函数声明
static void make_idt_desc(struct gate_desc * p_gdesc,uint8_t attr,intr_handler function);
static struct gate_desc idt[IDT_DESC_CNT];  //中断描述符表,本质上就是中段门描述符数组

char * intr_name[IDT_DESC_CNT]; //保存异常名
intr_handler idt_table[IDT_DESC_CNT];   //这是将来中断服务程序要调用的程序

extern intr_handler intr_entry_table[IDT_DESC_CNT]; //中断服务程序的入口地址数组intr_entry_table,在kernel.S中已经构造好了，就是各个终端服务子程序


//完成所有初始化工作
static void pic_init(void)
{
    //初始化主片
    outb(PIC_M_CTRL,0x11);      //PIC_M_CTRL主片的0x20端口 ICW1:边沿触发，级联8259，需要用到ICW4
    outb(PIC_M_DATA,0x20);      //PIC_M_DATA主片的0x21数据端口,设置起始的中断向量号，0x20-0x27

    outb(PIC_M_DATA,0x04);      //ICW3,设置哪些接的从片，这里是IR2接的从片
    outb(PIC_M_DATA,0x01);      //8086处理器,手动发送EOI
    //初始化从片
    outb(PIC_S_CTRL,0x11);      //设置从片，0xa1边沿触发，需要ICw4
    outb(PIC_S_DATA,0x28);      //起始的中断向量号0x28,主片每个后面都是级联的

    outb(PIC_S_DATA,0x02);      //这个是链接到主片的0x02引脚
    outb(PIC_S_DATA,0x01);      //x86模式,手动发送EOI

    //打开主片的IRO，现在设置的是MASK，MASK位1就被屏蔽了
    outb(PIC_M_DATA,0xfe);
    outb(PIC_S_DATA,0xff);

    put_str(" pic_init done\n");
}

//创建中断描述符
static void make_idt_desc(struct gate_desc * p_gdesc,uint8_t attr,intr_handler function)
{
    p_gdesc->func_offset_low_word = (uint32_t)function & 0x0000FFFF;
    p_gdesc->selector = SELECTOR_K_CODE;//指向内核的选择子
    p_gdesc->dcount = 0;
    p_gdesc->attribute = attr;//IDT_DESC_ATTR_DPL0描述符属性
    p_gdesc->func_offset_high_word = ((uint32_t)function & 0xFFFF0000) >> 16;
}

//初始化中断描述符表
static void idt_desc_init(void)
{
    int i;
    for(int i =0;i < IDT_DESC_CNT;i++)
    {
        make_idt_desc(&idt[i],IDT_DESC_ATTR_DPL0,intr_entry_table[i]);
    }
    put_str(" idt_desc_init done\n");
}

//通用中断处理函数
static void general_intr_handler(uint8_t vec_nr)
{
    if(vec_nr == 0x27 || vec_nr == 0x2f)    return;     //当IRQ7和IRQ15产生的伪中断，可以不理会
    set_cursor(0);

    int cursor_pos = 0;
    while(cursor_pos < 320)
    {
        put_char(' ');cursor_pos++;
    }

    set_cursor(0);
    put_str("!!!!!!!!!!!!!!!!!! excetion message begin !!!!!!!!!!!!!!!!!!!!!\n");
    set_cursor(88);
    put_str(intr_name[vec_nr]);
    if(vec_nr == 14)
    {
        int page_fault_vaddr = 0;
        asm("movl %%cr2,%0":"=r"(page_fault_vaddr));
        put_str("\npage fault addr is ");put_int(page_fault_vaddr);
    }
    put_str("!!!!!!!!!!!!!!!!!! excetion message end !!!!!!!!!!!!!!!!!!!!!\n");
    while(1);
}
//做中断处理函数注册，以及异常名称注册
static void exception_init(void)
{
    int i;
    for(i = 0;i < IDT_DESC_CNT;i++)
    {
        idt_table[i]=general_intr_handler;
        intr_name[i]="unkown";
    }

   intr_name[0] = "#DE Divide Error";
   intr_name[1] = "#DB Debug Exception";
   intr_name[2] = "NMI Interrupt";
   intr_name[3] = "#BP Breakpoint Exception";
   intr_name[4] = "#OF Overflow Exception";
   intr_name[5] = "#BR BOUND Range Exceeded Exception";
   intr_name[6] = "#UD Invalid Opcode Exception";
   intr_name[7] = "#NM Device Not Available Exception";
   intr_name[8] = "#DF Double Fault Exception";
   intr_name[9] = "Coprocessor Segment Overrun";
   intr_name[10] = "#TS Invalid TSS Exception";
   intr_name[11] = "#NP Segment Not Present";
   intr_name[12] = "#SS Stack Fault Exception";
   intr_name[13] = "#GP General Protection Exception";
   intr_name[14] = "#PF Page-Fault Exception";
   // intr_name[15] 第15项是intel保留项，未使用
   intr_name[16] = "#MF x87 FPU Floating-Point Error";
   intr_name[17] = "#AC Alignment Check Exception";
   intr_name[18] = "#MC Machine-Check Exception";
   intr_name[19] = "#XF SIMD Floating-Point Exception";
}

//开中断并返回开中断前的状态
enum intr_status intr_enable()
{
    enum intr_status old_status;
    if(INTR_ON == intr_get_status())
    {
        old_status = INTR_ON;
        return old_status;
    }
    else
    {
        old_status=INTR_OFF;
        asm volatile("sti");    //开中断，sti将IF设置为1
        return old_status;
    }
}

//关中断，并且返回关中断前的状态
enum intr_status intr_disable()
{
    enum intr_status old_status;
    if(INTR_ON == intr_get_status())
    {
        old_status = INTR_ON;
        asm volatile("cli":::"memory");//关中断，并且将IF位置零
        return old_status;
    }
    else
    {
        old_status=INTR_OFF;
        return old_status;
    }
}

void register_handler(uint8_t vector_no,intr_handler function)
{
    idt_table[vector_no]=function;
}

//将中断状态设置为status
enum intr_status intr_set_status(enum intr_status status)
{
    return status & INTR_ON ? intr_enable() : intr_disable();
}

//获取当前中断状态
enum intr_status intr_get_status()
{
    uint32_t eflags = 0;
    GET_EFLAGS(eflags);
    return (EFLAGS_IF & eflags)?INTR_ON:INTR_OFF;
}

//完成有关中断的所有初始化工作
void idt_init()
{
    put_str("idt_init start\n");
    idt_desc_init();
    exception_init();
    pic_init();

    // //加载idt  idt存一个48位的数据，低16位是有多少个中断，高32位是中断描述符表的基址
    uint64_t idt_operand = ((sizeof(idt) - 1) | ((uint64_t)(uint32_t)idt << 16));
    asm volatile("lidt %0" : : "m" (idt_operand));
    put_str("idt_init done\n");
}