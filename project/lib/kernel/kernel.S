[bits 32]
%define ERROR_CODE nop          ;这两个宏保证了在中断处理的栈空间中，EIP退回前都含有一个错误码
%define ZERO push 0             ;若相关的异常中没有压入错误码，wile统一格式，就手动压入一个0

extern push_str                 ;声明外部函数

section .data
intr_str db "interrupt occur!",0xa,0
global intr_entry_table
intr_entry_table:

%macro VECTOR 2                 ;VECTOR表示宏名称 后面表示参数个数%1 %2
section .text                   
intr%1entry:
    %2                          
    push intr_str               ;先打印进入中断的信息
    call put_str
    add esp,4

    mov al,0x20                 ;发送EOI信号
    out 0xa0,al
    out 0x20,al

    add esp,4
    iret
section .data
    dd intr%1entry
%endmacro

VECTOR 0x0 ,ZERO
VECTOR 0X1 ,ZERO
VECTOR 0X2 ,ZERO
VECTOR 0x3 ,ZERO
VECTOR 0X4 ,ZERO
VECTOR 0X5 ,ZERO
VECTOR 0x6 ,ZERO
VECTOR 0X7 ,ZERO
VECTOR 0X8 ,ERROR_CODE
VECTOR 0x9 ,ZERO
VECTOR 0XA ,ERROR_CODE
VECTOR 0XB ,ERROR_CODE
VECTOR 0XC ,ERROR_CODE
VECTOR 0XD ,ERROR_CODE
VECTOR 0XE ,ERROR_CODE
VECTOR 0XF ,ZERO
VECTOR 0X10 ,ZERO
VECTOR 0X11 ,ERROR_CODE
VECTOR 0x12 ,ZERO
VECTOR 0X13 ,ZERO
VECTOR 0X14 ,ZERO
VECTOR 0x15 ,ZERO
VECTOR 0X16 ,ZERO
VECTOR 0X17 ,ZERO
VECTOR 0X18 ,ZERO
VECTOR 0X19 ,ZERO
VECTOR 0X1A ,ZERO
VECTOR 0X1B ,ZERO
VECTOR 0X1C ,ZERO
VECTOR 0X1D ,ZERO
VECTOR 0X1E ,ERROR_CODE                               ;处理器自动推错误码
VECTOR 0X1F ,ZERO
VECTOR 0X20 ,ZERO