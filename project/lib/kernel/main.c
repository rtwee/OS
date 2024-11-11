//将来被指定入口地址在0xc0001500
//ld main.o -Ttext 0xc0001500 -e main -o kernel.bin
//ld来链接程序  -Ttext 指定起始的虚拟地址 -e 表示的入口地址 w我们用main函数，因此使用-e main
//如果不指定入口地址，则会使用默认的_start,因此如果把main函数名改成_start,即使不指定入口地址也是可以运行的
#include "print.h"

int main(void)
{
    put_str("hello I am a kernel\n");
    put_int(0);
    put_char('\n');
    put_int(9);
    put_char('\n');
    put_int(0x00021a3f);
    put_char('\n');
    put_int(0x12345678);
    put_char('\n');
    put_int(0x00000000);
    while(1);
    return 0;
}