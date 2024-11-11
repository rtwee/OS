%include "boot.inc"
SECTION loader vstart=LOADER_BASE_ADDR
LOADER_STACK_TOP equ LOADER_BASE_ADDR

jmp loader_start

GDT_BASE:
    dd 0x00000000
    dd 0x00000000
CODE_DESC:
    dd 0x0000FFFF
    dd DESC_CODE_HIGH4
DATA_STACK_DESC:
    dd 0x0000FFFF
    dd DESC_DATA_HIGH4
VIDEO_DESC:
    dd 0x80000007
    dd DESC_VIDEO_HIGH4

    GDT_SIZE  equ $ - GDT_BASE
    GDT_LIMIT equ GDT_SIZE - 1
;这个对齐数要算一下，书又错了
    times 59 dq 0
    times 5 db 0
;---------------------------------------
;将来要存放的总内存大小
    total_mem_byte dd 0

; ;选择子一代表的是代码段
;     SELECTOR_CODE  equ (0x0001<<3) + TI_GDT + RPL0
;     SELECTOR_DATA  equ (0x0002<<3) + TI_GDT + RPL0
;     SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT + RPL0
; ;将来要加载的段描述表
    gdt_ptr dw GDT_LIMIT
            dd GDT_BASE

;     loadermsg db '2 loader in real.'

;人工对齐total_mem_byte 4 + gdt_ptr 6 + buffer244 + nr2 == 256字节
    ards_buf times 244 db 0 
;用于记录ARDS结构体数量
    ards_nr dw 0

;选择子一代表的是代码段
    SELECTOR_CODE  equ (0x0001<<3) + TI_GDT + RPL0
    SELECTOR_DATA  equ (0x0002<<3) + TI_GDT + RPL0
    SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT + RPL0

loader_start:
;初始化一下自己的栈指针
    mov sp,LOADER_BASE_ADDR
;----------------------------------------------调用第一种中断来获取内存的大小,中断号码0xe820
    ;int0x15 eax=0000E820h,edx=534D4150h('SMAP')
    xor ebx,ebx         
    mov edx,0x534d4150
    mov di,ards_buf
.e820_mem_get_loop:
;每次执行的时候都需要加上中断的子功能号
    mov eax,0x0000e820
;ARDS地址范围描述符结构是20字节
    mov ecx,20
    int 0x15
;若失败则CF位设为1
    jc .e820_faild_so_try_e801
;成功后开始统计内存范围
    add di,cx           ;di增加20字节就是只想下一个ARDS的位置
    inc word [ards_nr]  ;ARDS的数量加1
    cmp ebx,0           ;若ebx为0，说明当前的ards全部返回了
    jnz .e820_mem_get_loop  ;说明当前的段信息还没有获取全

;下面找到base_add_low+length_low的最大值就是内存的容量
    mov cx,[ards_nr]
    mov ebx,ards_buf
    xor edx,edx
.find_max_mem_area:
    mov eax,[ebx]       ;baseaddr_low
    add eax,[ebx+8]     ;length_low
    add ebx,20          ;移动到下一个ADRS
    cmp edx,eax         ;比较ebx和当前里面的eax哪个大
    jge .next_ards
    mov edx,eax         ;如果当前的大那就进行替换
.next_ards:
    loop .find_max_mem_area ;cx就是ards的数量等待所有的都遍历完
    jmp .mem_get_ok


;----------------------------------------------调用第二种中断来获取内存的大小,中断号码0xe801
.e820_faild_so_try_e801:
    mov ax,0xe801
    int 0x15 
    jc .e801_failed_so_try88

;1.先计算地端的15M内存
    mov cx,0x400    ;其实就是乘以1024
    mul cx
    shl edx,16      ;把低16位清零
    and eax,0x0000FFFF
    or edx,eax
    add edx,0x100000    ;ax只有15MB，要在加1MB
    mov esi,edx         ;把低端的内存存到esi寄存器中备份
;2.计算16MB以上的空间
    xor eax,eax
    mov ax,bx
    mov ecx,0x10000     ;十进制的64K
    mul ecx             ;这样高位存在edx，低位存在eax

    add esi,eax         ;加上当前计算出的大小
    mov edx,esi
    jmp .mem_get_ok

;----------------------------------------------调用第三种中断来获取内存的大小,中断号码0x88
.e801_failed_so_try88:
    mov ah,0x88
    int 0x15
    jc .error_hlt
    and eax,0x0000FFFF

    mov cx,0x400
    mul cx
    shl edx,16
    or edx,eax
    add edx,0x100000

.error_hlt:
    jmp $

.mem_get_ok:
    mov [total_mem_byte],edx


;-----------------------开始保护模式，首先装载保护模式下的段描述符 GDT
;-----------------------正式开始进入保护模式
;1.打开A20
;2.加载gdt
;3.将cr0的pe位置1
;-----------------------打开A20
    in al,0x92
    or al,0000_0010B
    out 0x92,al

;-----------------------加载GDT
    lgdt [gdt_ptr]

;-----------------------cr0第0位置1
    mov eax,cr0
    or eax,0x00000001
    mov cr0,eax

    jmp dword SELECTOR_CODE:p_mode_start            ;刷新流水线


[bits 32]
p_mode_start:
    mov ax,SELECTOR_DATA
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov esp,LOADER_STACK_TOP
;-----------------------------------------------加载内核到缓冲区------------------------------------
    mov eax,KERNEL_BIN_SECTOR
    mov ebx,KERNEL_BIN_BASE_ADDR

    mov ecx,200

    call rd_disk_m_32

;-----------------------------------------------启动分页----------------------------------------------
;1.准备好页表
    call setup_page
    ;将存储的GDT信息倒出来,放在gdt_ptr处
    sgdt [gdt_ptr]                  ;存储原来的gdt的所有位置

;将gdt描述符中视频段描述符中的段基址+0xc0000000 ;修改选段描述符，现在这个段描述符表示的就是虚拟地址，对于显卡区域放在内核是比较合适的
    mov ebx,[gdt_ptr + 2]

;这是它的高字节，相当于让他和内核的基地址相加   ;视频段是第三个描述符，每个描述符占用8个字节  ;段描述符的高四字节的最高位是段基址的第31-24位
    or dword [ebx + 0x18 + 4],0xc0000000

;将gdt的基址加上0xc0000000使其成为内核所在的高位地址，映射到内核空间    
    add dword [gdt_ptr + 2],0xc0000000
    add esp,0xc0000000

;2.把页表地址赋值给cr3
    mov eax,PAGE_DIR_TABLE_POS
    mov cr3,eax

;3.打开cr0的pg位（第31位)
    mov eax,cr0
    or eax,0x80000000
    mov cr0,eax

;开启分页后，用gdt新的地址重新加载
    lgdt [gdt_ptr]

    jmp SELECTOR_CODE:enter_kernel

;--------------------------------------跳转到内核区域-------------------------------------
enter_kernel:
    call kernel_init

    mov eax,SELECTOR_VIDEO
    mov gs,eax
    mov byte [gs:160],'V'

    ;我们的缓冲区区域范围是0x700000~0x9f0000范围
    mov esp,0xc009f000
    jmp KERNEL_ENTER_ADDR
    ; jmp $

;--------------------- 创建页目录和页表 -----------------------
;1.把4K个byte进行情况
setup_page:
    mov ecx,4096
    mov esi,0
.clear_page_dir:
    mov byte [PAGE_DIR_TABLE_POS + esi],0
    inc esi
    loop .clear_page_dir
;2.开始创建页目录项PDE
.create_pde:
    mov eax,PAGE_DIR_TABLE_POS
    add eax,0x1000
    or eax,PG_US_U | PG_RW_W | PG_P     ;下面把eax这个地址设置撑页表项的格式，也就是把第一个页表存在第一个目录项中

    mov [PAGE_DIR_TABLE_POS+0x0],eax    ;在第一项中存放第一个页表项
    mov [PAGE_DIR_TABLE_POS+0xc00],eax  ;在0xc00以上是属于内核空间的

    sub eax,0x1000                      ;现在表示的就是第一个页目录了
    mov [PAGE_DIR_TABLE_POS + 4092],eax ;页目录表的最后一项放的是自己

;3.创建PTE
    mov eax,PAGE_DIR_TABLE_POS
    add eax,0x1000
    mov ecx,256                         ;目前我们只使用低1M的内存，页大小是4K，所以只需要创建256个就可以了
    mov esi,0
    mov ebx,PG_US_U | PG_RW_W | PG_P    ;也就是给第一个页表的地址低位设置
.create_pte:
    mov [eax + esi*4],ebx

    add ebx,4096
    inc esi
    loop .create_pte
;创建内核其他页表的PDE
    mov eax,PAGE_DIR_TABLE_POS
    add eax,0x2000
    or eax,PG_US_U | PG_RW_W | PG_P
    mov ebx,PAGE_DIR_TABLE_POS
    mov ecx,254
    mov esi,769
.create_kernel_pde:
    mov [ebx+esi*4],eax
    inc esi
    add eax,0x1000
    loop .create_kernel_pde
    ret

;---------------------------------------------初始化内核---------------------------------------------------
kernel_init:
    xor eax,eax
    xor ebx,ebx
    xor ecx,ecx
    xor edx,edx

    mov ebx,[KERNEL_BIN_BASE_ADDR+28]
    add ebx,KERNEL_BIN_BASE_ADDR        ;ebx存放的是程序段表的起始地址

    mov dx,[KERNEL_BIN_BASE_ADDR+42]    ;获取每个程序段表条目描述符的字节大小
    mov cx,[KERNEL_BIN_BASE_ADDR+44]    ;获取共包含多个个段

.get_each_sgment:
    ;如果当前的程序段表为空，则直达PTNULL
    cmp dword [ebx+0],PT_NULL
    je .PTNULL

    mov eax,[ebx+8]                 ;p_vadder起始的虚拟地址
    cmp eax,KERNEL_BIN_BASE_ADDR    ;如果比起始地址还小那就说明有问题
    jb .PTNULL
;   memcpy(dst,src,size)
    push dword [ebx+16]             ;本段在文件中的大小 memcpy的第三个参数size

    mov eax,[ebx+4]                 ;本段在文件中的偏移
    add eax,KERNEL_BIN_BASE_ADDR    ;本段的开始位置
    push eax                        ;memcpy的第二个参数 src
    push dword [ebx+8]              ;存储目的地址   p_vadder起始的虚拟地址

    call mem_cpy
    add esp,12                      ;清理栈

.PTNULL:
    add ebx,edx                     ;dx中放的是program header的大小,此时就要将ebx移动到下一个程序头表的位置
    loop .get_each_sgment
    ret

mem_cpy:
    cld         ;清楚方向寄存器的值
    push ebp    ;ebp先存一下
    mov ebp,esp ;esp指针赋值给ebp
    push ecx    ;ecx是段的数量，入栈

    mov edi,[ebp + 8]   ;指向p_vadder
    mov esi,[ebp + 12]  ;指向src
    mov ecx,[ebp + 16]  ;指向size

    rep movsb

    pop ecx
    pop ebp
    ret

;--------------------------------------------- rd_disk_m_32  ---------------------------------------------
;1.选择通道，往通道的sector count寄存器中写入待操作的扇区数量 ***对应第一步骤中的0x1f2给主盘的count寄存器传入尧都区的扇区数为cl
;2.往该通道的三个LBA寄存器写入扇区起始位置低24位
rd_disk_m_32:
    mov esi,eax                     ;eax=LBA扇区号,0x2
    mov di,cx                       ;写入的山区数量位cx，在di中备份
;第一步设置要读取的扇区数量 主盘0x1f2
    mov dx,0x1f2
    mov al,cl                       
    out dx,al                       ;要读取的数量
    mov eax,esi                     ;恢复ax
;第二步写入LBA地址  0x1f3-0x1f5为低到高的24位
    mov dx,0x1f3
    out dx,al                       ;将2写道LBA的低地址

    mov cl,8
    shr eax,cl                      ;右边移动8位
    mov dx,0x1f4
    out dx,al

    shr eax,cl
    mov dx,0x1f5
    out dx,al

    shr eax,cl
    and al,0x0f                     ;25-18位是在0x1f6中的下半部分这是device寄存器
    or al,0xe0                      ;将4-7位置进行设置1110,表示lba模式
    mov dx,0x1f6
    out dx,al
;第三步 向0x1f7端口写入读命令,0x20
    mov dx,0x1f7
    mov al,0x20
    out dx,al
;第四步：检测硬盘状态
.not_ready:
    nop
    in al,dx
    and al,0x88                      ;第七位是BUSY位置，第四位是有效位，成功后设置为1
    cmp al,0x08
    jnz .not_ready

;第五步：从0x1f0读取数据
    mov ax,di                        ;di表示要写入的扇区数量
    mov dx,256
    mul dx
    mov cx,ax                        ;一个扇区又512个字节，每次读入16位，因此读入 256 * count次

    mov dx,0x1f0

.go_on_read:
    in ax,dx
    mov [ebx],ax                      ;bx放的是指定的位置，这个位置是内存中空的区域
    add ebx,2
    loop .go_on_read
    ret