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

VECTOR 0x00,ZERO
VECTOR 0x01,ZERO
VECTOR 0x02,ZERO

VECTOR 0x1e,ERROR_CODE
VECTOR 0x1f,ZERO