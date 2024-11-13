#include "interrupt.h"
#include "stdint.h"
#include "global.h"
#include "io.h"

#define IDT_DESC_CNT 0x21       //目前总共支持的中断数量

#define PIC_M_CTRL 0x20         //主片控制端口
#define PIC_M_DATA 0x21         //主片数据端口
#define PIC_S_CTRL 0xa0         //从片控制端口
#define PIC_S_DATA 0xa1         //从片数据端口


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
// static void make_idt_desc(struct gate_desc * p_gdesc,uint8_t attr,intr_handler function);
static struct gate_desc idt[IDT_DESC_CNT];  //中断描述符表,本质上就是中段门描述符数组
extern intr_handler intr_entry_table[IDT_DESC_CNT]; //中断服务程序的入口地址数组intr_entry_table,在kernel.S中已经构造好了，就是各个终端服务子程序


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

//完成所有初始化工作
static void pic_init(void)
{
    //初始化主片
    

    //初始化从片
}


//完成有关中断的所有初始化工作
void idt_init()
{
    put_str("idt_init start\n");
    idt_desc_init();
    pic_init();

    //加载idt
    uint64_t idt_operand = ((sizeof(idt) - 1) | ((uint64_t)((uint32_t)idt << 16)));
    asm volatile("lidt %0"::"m"(idt_operand));
    put_str("idt_init done\n");
}