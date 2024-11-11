section .data
str_c_lib: db "C libary say:hello world!",0xa   ;0xa是换行符
str_c_lib_len equ $-str_c_lib

str_syscall: db "syscall says:hello world!",0xa
str_syscall_len equ $-str_syscall

section .text
global _start
_start:
;;;;;;;;;;;;;;;;;;;;;;;用c语言模拟系统调用库函数write;;;;;;;;;;;;;;;;;;;;;;;;;
    push str_c_lib_len  ;write(fd,buf,count) 从右到左传递
    push str_c_lib
    push 1

    call simu_write
    add esp,12          ;弹出这三个参数

;;;;;;;;;;;;;;;;;;;;;;;跨过库函数直接调用;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    mov eax,4           ;write系统功能号是4
    mov ebx,1
    mov ecx,str_syscall
    mov edx,str_syscall_len
    int 0x80
;;;;;;;;;;;;;;;;;;;;;;;退出程序;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    mov eax,1           ;退出的子功能号是exit
    int 0x80            ;发起中断,通知linux完成请求的功能
simu_write:
    push ebp
    mov ebp,esp
    mov eax,4
    mov ebx,[ebp+8]
    mov ecx,[ebp+12]
    mov edx,[ebp+16]
    int 0x80
    pop ebp
    ret 