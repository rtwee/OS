;--------------------loader 和 kernel----------------------
;LOADER_BASE_ADDR表示将磁盘中代码拷贝到内存的位置
;LOADER_START_SECTOR表示磁盘的LBA,就是第几个扇区，低0个扇区放的mbr现在第二个扇区放的loader
LOADER_BASE_ADDR equ 0x900
LOADER_START_SECTOR equ 0x2

;这表示了页目录表的起始地址
PAGE_DIR_TABLE_POS equ 0x100000

;--------------------gdt描述符属性--------------------------

;DESC_G_4K表示单位是4K，有20位来限定段的大小，从-0位开始算，第24位表示的是单位1为4K，0为1字节，这样能表示4G范围和1M范围
DESC_G_4K equ 1_00000000000000000000000b
;DESC_D_32表示的操作寄存器，机器码啥的是32位的，在第23位
DESC_D_32 equ 1_0000000000000000000000b
;取零表示用32位，但是取1表示用的64位
DESC_L    equ 0_000000000000000000000b
;表示是否有效，对硬件来说这个设置没啥用
DESC_AVL  equ 0_00000000000000000000b

;下面16-19四位表示的范围
DESC_LIMIT_CODE2  equ 1111_0000000000000000b
DESC_LIMIT_DATA2  equ DESC_LIMIT_CODE2
;显卡的范围只有很小一部分用不到这个高位
DESC_LIMIT_VIDEO2 equ 0000_0000000000000000b

;在内存中是否存在
DESC_P  equ 1_000000000000000b
;特权等级
DESC_DPL_0 equ 00_0000000000000b
DESC_DPL_1 equ 01_0000000000000b
DESC_DPL_2 equ 10_0000000000000b
DESC_DPL_3 equ 11_0000000000000b
;表示系统段还是普通段
DESC_S_CODE equ 1_000000000000b
DESC_S_DATA equ DESC_S_CODE
DESC_S_Sys  equ 0_000000000000b

;表示属性可执行，一致性还有个保留位
;x=1,c=0,r=0,a=0
DESC_TYPE_CODE equ 1000_00000000b
;x=0,e=0,w=1,a=0 可写
DESC_TYPE_DATA equ 0010_00000000b

DESC_CODE_HIGH4 equ (0x00 << 24) + DESC_G_4K + DESC_D_32 + DESC_L + DESC_AVL + DESC_LIMIT_CODE2 + DESC_P +DESC_DPL_0 +DESC_S_CODE +DESC_TYPE_CODE + 0x00
DESC_DATA_HIGH4 equ (0x00 << 24) + DESC_G_4K + DESC_D_32 + DESC_L + DESC_AVL + DESC_LIMIT_DATA2 + DESC_P +DESC_DPL_0 +DESC_S_DATA +DESC_TYPE_DATA + 0x00
DESC_VIDEO_HIGH4 equ (0x00 << 24) + DESC_G_4K + DESC_D_32 + DESC_L + DESC_AVL + DESC_LIMIT_VIDEO2 + DESC_P +DESC_DPL_0 +DESC_S_DATA +DESC_TYPE_DATA + 0x0B  ;书上这块写错了显卡的地址不对

;-------------- 选择子属性 ---------
RPL0 equ 00b
RPL1 equ 01b
RPL2 equ 10b 
RPL3 equ 11b 
TI_GDT equ 000b 
TI_LDT equ 100b 


;-------------- 页表相关属性 ----------
;页表的高20位为地址，因为乘以4K的单位，所以高20位就能表示所有的空间范围
;P表示是否在内存中存在
PG_P equ 1b 
PG_RW_R equ 00b 
PG_RW_W equ 10b 
;超级用户或者是普通用户
PG_US_S equ 000b 
PG_US_U equ 100b


;------------------------加载内核的相关宏-------------------------------
KERNEL_BIN_SECTOR    equ 0x9            ;表示程序放在磁盘的第九个位置
KERNEL_BIN_BASE_ADDR equ 0x70000        ;缓冲区的起始位置
KERNEL_ENTER_ADDR    equ 0xc0001500    ;指定的内核的虚拟入口地址

PT_NULL               equ 0x0            ;表示当前的程序头为空