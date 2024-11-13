#ifndef __LIB_IO_H
#define __LIB_IO_H
#include "stdint.h"
/*
    机器模式： 
    b--输出寄存器QImode名称，寄存器中的最低8位:[a-d]l
    w--输出寄存器HImode名称，寄存器中的2字节的,[a-d]x
*/

//向端口指定N表示0-255,d表示用dx存储端口号
static inline void outb(uint16_t port,uint8_t data)
{
    //"a"是用al寄存器，Nd是指定端口N到dx寄存器
    asm volatile( "outb %b0,%w1" : : "a"(data),"Nd"(port));
}

//将addr处起始的word_cnt个字写道端口port中
static inline void outsw(uint16_t port,const void * addr,uint32_t word_cnt)
{
    /*
        这里面用到了+，+表示即是输入又是输出
        +S,使用esi，要读要写是+
        +c，使用cx，要读要写是+
        d，是dx
    */
   asm volatile("cld;rep outsw":"+S"(addr),"+c"(word_cnt):"d"(port));
}

//从端口port读取一个字节返回
static inline uint8_t inb(uint16_t port)
{
    uint8_t data;
    asm volatile("inb %w1,%b0":"a"(data):"Nd"(port));
    return data;
}

//将port读取word_cnt个字写入addr
static inline void insw(uint16_t port,void * addr,uint32_t word_cnt)
{
    /*
        将端口port处读入的16为内容写入到edi中
    */
    asm volatile("cld;rep insw":"+D"(addr),"+c"(word_cnt):"d"(port):"memory");
}
#endif