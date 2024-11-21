;主引导程序
;-----------------------------------------------
%include "boot.inc"
SECTION MBR vstart=0x7c00
    mov ax,cs
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov fs,ax
    mov sp,0x7c00
;这是显卡的地址
    mov ax,0xb800
    mov gs,ax

;利用ah=0x06 清屏
    mov ax,0x600
    mov bx,0x700
    mov cx,0
    mov dx,0x184f

    int 0x10
;输出字符串MBR
    mov byte [gs:0x00],'h'
    mov byte [gs:0x01],0xA4

    mov byte [gs:0x02],'e'
    mov byte [gs:0x03],0xA4

    mov byte [gs:0x04],'l'
    mov byte [gs:0x05],0xA4

    mov byte [gs:0x06],'l'
    mov byte [gs:0x07],0xA4

    mov byte [gs:0x08],'o'
    mov byte [gs:0x09],0xA4

    mov eax,LOADER_START_SECTOR    ;起始扇区的lba地址 是2
    mov bx,LOADER_BASE_ADDR         ;写入的地址
    mov cx,4                        ;等待写入的扇区数

;将扇区中的内容读出，并写道LOADER_BASE_ADDR这个位置
    call rd_disk_m_16
;跳转到这个位置
    jmp LOADER_BASE_ADDR

;---------------------功能读取硬盘的cx个扇区--------------------------
;1.选择通道，往通道的sector count寄存器中写入待操作的扇区数量 ***对应第一步骤中的0x1f2给主盘的count寄存器传入尧都区的扇区数为cl
;2.往该通道的三个LBA寄存器写入扇区起始位置低24位
rd_disk_m_16:
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
    mov [bx],ax                      ;bx放的是指定的位置，这个位置是内存中空的区域
    add bx,2
    loop .go_on_read
    ret

    times 510-($-$$) db 0
    db 0x55,0xaa
