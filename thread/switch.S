[bits 32]
section .text
global switch_to
switch_to:
    ;栈中此处是返回地址
    push esi
    push edi
    push ebx
    push ebp
    mov eax,[esp + 20]  ;得到栈中的参数cur，cur=esp+20
    mov [eax],esp

    mov eax,[esp+24]    ;得到next参数
    mov esp,[eax]

    pop ebp
    pop ebx
    pop edi
    pop esi
    ret