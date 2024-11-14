#!/bin/bash
compile_loader()
{
    echo `nasm -I /home/cs/bochs/project/lib/boot -o /home/cs/bochs/project/lib/boot/loader.bin /home/cs/bochs/project/lib/boot/loader.S`
    echo `dd if=/home/cs/bochs/project/lib/boot/loader.bin of=/home/cs/bochs/bin/hd60M.img bs=512 count=3 seek=2 conv=notrunc`
}

compile_kernel()
{
    
    gcc -m32 -I /home/cs/bochs/project/lib/kernel  -I /home/cs/bochs/project/lib/user -c -fno-builtin -o /home/cs/bochs/project/lib/kernel/main.o /home/cs/bochs/project/lib/kernel/main.c -fno-stack-protector

    
    gcc -m32 -I /home/cs/bochs/project/lib/kernel  -I /home/cs/bochs/project/lib/user -c -fno-builtin -o /home/cs/bochs/project/lib/kernel/interrupt.o /home/cs/bochs/project/lib/kernel/interrupt.c -fno-stack-protector
    gcc -m32 -I /home/cs/bochs/project/lib/kernel  -I /home/cs/bochs/project/lib/user -c -fno-builtin -o /home/cs/bochs/project/lib/kernel/init.o /home/cs/bochs/project/lib/kernel/init.c -fno-stack-protector

    nasm -f elf -o /home/cs/bochs/project/lib/kernel/print.o /home/cs/bochs/project/lib/kernel/print.S
    nasm -f elf -o /home/cs/bochs/project/lib/kernel/kernel.o /home/cs/bochs/project/lib/kernel/kernel.S

    
    ld -m elf_i386 -Ttext 0xc0001500 -e main -o /home/cs/bochs/project/lib/kernel/kernel.bin /home/cs/bochs/project/lib/kernel/main.o /home/cs/bochs/project/lib/kernel/print.o /home/cs/bochs/project/lib/kernel/interrupt.o /home/cs/bochs/project/lib/kernel/init.o /home/cs/bochs/project/lib/kernel/kernel.o
    dd if=/home/cs/bochs/project/lib/kernel/kernel.bin of=/home/cs/bochs/bin/hd60M.img bs=512 count=200 seek=9 conv=notrunc
}


run()
{
    compile_loader
    compile_kernel
    #这里可以更改
    /home/cs/bochs/bin/bochs -f /home/cs/bochs/bin/bochsrc.disk
}

run