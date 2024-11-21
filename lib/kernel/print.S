TI_GDT equ 0
RPL0   equ 0

SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT +RPL0

section .data
put_int_buffer dq 0 ;八字节

[bits 32]
section .text
;--------------------- put str  -------------------------
;功能描述，使用下面的put char 来打印字符串
;--------------------------------------------------------
global put_str
put_str:
    push ebx
    push ecx        ;ebx指向打印字符串的首地址
    xor ecx,ecx     ;exc用来放要打印的字符
    mov ebx,[esp + 12];字符串地址，压栈地址，ebx,ecx
.goon:
    mov cl,[ebx]
    cmp cl,0
    jz .str_over    ;如果当前是结束符
    push ecx
    call put_char
    add esp,4       ;丢弃当前入栈的ecx
    inc ebx
    jmp .goon
.str_over:
    pop ecx
    pop ebx
    ret

;--------------------- put char -------------------------
;功能描述，把栈中的1个字符写到光标处
;--------------------------------------------------------
global put_char
put_char:
    pushad          ;备份32位寄存器环境
    ;需要保证gs中为正确的是频段选择子
    ;为保险起见，每次打印都为gs赋值
    mov ax,SELECTOR_VIDEO   ;立即数无法直接给段寄存器中传入
    mov gs,ax

;;;;;;;;;;;;;;;;;;;;;;; 获得当前光标的位置 ;;;;;;;;;;;;;;;;;;;;;;
    ;先获取 高8位
    mov dx,0x03d4           ;索引寄存器
    mov al,0x0e             ;用于提供光标位置的高8位
    out dx,al
    mov dx,0x03d5           ;通过读写数据端口0x3d5来获得或设置光标位置
    in al,dx
    mov ah,al

    ;在获取 低8位
    mov dx,0x03d4
    mov al,0x0f
    out dx,al
    mov dx,0x03d5
    in al,dx

    ;将光标存入bx
    mov bx,ax
    ;下面这行实在栈中获取等待打印的字符，pushad压入了4*8字节  + 还有4字节的返回地址
    mov ecx,[esp + 36]
    ;CR==0x0d,LF==0x0a,BS==0x08

    cmp cl,0xd
    jz .is_carriage_return
    cmp cl,0xa
    jz .is_line_feed

    cmp cl,0x8
    jz .is_backspace
    jmp .put_other

.is_backspace:
;;;;;;;;;;;;;;;;;;;;用来退格;;;;;;;;;;;;;;;;;;;;;
;空格只需要将光标向前移动一个显存位置就好了,但是有时候光标移动后还在原处，会比较奇怪，因此添加了空格或空字符0
    dec bx              ;bx自减1，先减了1说明回退到前一个位置
    shl bx,1            ;光标左移1位，相当于乘以2；由于现存中每个位置占用两个字节，所以光标位置的值乘以2，就是它在显存中的相对位置

    mov byte [gs:bx],0x20   ;先把空格的ascii码写道光标的位置
    inc bx                  ;bx增加1，指向了属性设置位
    mov byte [gs:bx],0x07   ;将属性设置，0x7表示黑屏白字
    shr bx,1                ;右移1，表示除2，这样bx又恢复了，表示的是光标的地址
    jmp .set_cursor         ;设置光标的位置bx
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.put_other:
    shl bx,1                ;光标位置用两个字节表示，将光标值乘以2,表示对应现存中的偏移字节

    mov byte [gs:bx],cl     ;设置要打印的字符
    inc bx                  ;到属性位
    mov byte [gs:bx],0x07   ;属性设置
    shr bx,1                ;恢复光标位置
    inc bx                  ;移动到下一个光标位置
    cmp cx,2000             ;若小于2000，表示没有超出内存，则去设置给黄标，若超出则进行换行处理
    jl .set_cursor

.is_line_feed:              ;LF换行符
.is_carriage_return:        ;CR回车符
    xor dx,dx               ;清除dx
    mov ax,bx               ;ax是除数的低16位
    mov si,80               ;25*80行

    div si                  ;得到第几行，和余数
    sub bx,dx               ;减去余数了这样久得到要回车的光标位置了

.is_carriage_return_end:
    add bx,80               ;给光标位置加一行，也就是到下一行开头
    cmp bx,2000             ;比较是否超出了
.is_line_feed_end:  
    jl .set_cursor          ;如果没有超出当前分为，那就更新光标位置
    ;如果超出这个范围则表示目前需要滚屏了,
    ;(1)当最后一个位置超过范围，或者最后一行要回车，那就需要滚屏
    ;0xc 和0xd设置高八位和低八位的起始地址，显示器会根据这个起始地址向后显示2000个字符
    ;如果数字超过了范围，那就会回绕（最多有8个页面）
    ;(2)第二种方案就是我们直接固定是从0位置开始读写的,为了方便，我们使用第二种方案,步骤如下:
    ;a.将1-24行的内容搬移到0-23行
    ;b.将第24行用 空格 覆盖
    ;c.把光标移动到最后一行行首
.roll_screen:
    cld                     ;清空方向寄存器
    mov ecx,960             ;2000-80个字符需要搬运，移动1920*2个字节，每次搬运4字节 960次

    mov esi,0xc00b80a0      ;第一行的行首(这是虚拟地址了)
    mov edi,0xc00b8000      ;第0行行首(这是虚拟地址了)
    rep movsd
    ;将最后一行设位空格
    mov ebx,3840            ;这就是最后一行的开头位置
    mov ecx,80              ;需要做一行就是80次
.cls:
    mov word [gs:ebx],0x0720;别忘了每个是2字节，同时设置属性和文本
    add ebx,2
    loop .cls
    mov bx,1920             ;将光标位置重置到这一行的开头
.set_cursor:
;;;;;;;;;;;;;;1 先设置高8位置 ;;;;;;;;;;;;;;;;;;
    mov dx,0x03d4
    mov al,0x0e
    out dx,al
    mov dx,0x03d5           ;读写这个端口就是获得和设置光标的方法
    mov al,bh
    out dx,al
;;;;;;;;;;;;;;1 先设置高8位置 ;;;;;;;;;;;;;;;;;;
    mov dx,0x03d4
    mov al,0x0f
    out dx,al
    mov dx,0x03d5           ;读写这个端口就是获得和设置光标的方法
    mov al,bl
    out dx,al
.put_char_done:
    popad
    ret

;;;;;;;;;;;;;;;;;将小端字节序的数字变成对应的ASCII后，倒置;;;;;;;;;;;;
;栈中参数为待打印数字，输出为十六进制数字，但是不显示0x

global put_int
put_int:
    pushad
    mov ebp,esp
    mov eax,[ebp+4*9]   ;eax中放的是备份
    mov edx,eax         ;ebx放的每次参与位数转换的数据
    mov edi,7           ;指定再put_int_buffer的偏移量(7表示最后一个字节（总共八个，0-7）)
    mov ecx,8           ;32位数，每四位表示一个字符，也就是说一共有八个
    mov ebx,put_int_buffer

    ;将32位数按照十六进制的形式从高位到低位逐个处理
.16based_4bits:
    and edx,0x0000000F  ;解析十六进制的每一位(拿到最后一个位的值)
    cmp edx,9           ;数字0-9和a-f分别处理成对应的字符
    jg .isA2F
    add edx,'0'         ;给他加上0字符即可
    jmp .store
.isA2F:
    sub edx,10          ;A-F先要减去10
    add edx,'A'
    ;此时dl中放的是字符
.store:
    mov [ebx+edi],dl       
    dec edi             ;7 6 5 ... 0顺序处理
    shr eax,4           ;将下一个处理位赋值
    mov edx,eax
    loop .16based_4bits

;现在put_int_buffer中全部都是字符
.ready_to_print:
    inc edi
.skip_prefix_0:
    cmp edi,8

    je .full0                   ;全0则返回0
.go_on_skip:
    mov cl,[put_int_buffer+edi]
    inc edi
    cmp cl,'0'
    je .skip_prefix_0           ;跳过前面为0的数字
    dec edi                     ;让edi恢复当前的位置
    jmp .put_each_num

.full0:
    mov cl,'0'
.put_each_num:
    push ecx
    call put_char
    add esp,4
    inc edi
    mov cl,[put_int_buffer+edi]
    cmp edi,8
    jl .put_each_num
    popad
    ret