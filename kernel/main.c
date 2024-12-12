# include "print.h"
# include "init.h"
# include "thread.h"
# include "interrupt.h"
# include "console.h"
# include "ioqueue.h"
# include "keyboard.h"

void k_thread_function_a(void*);
void k_thread_function_b(void*);

int main(void) {
    // 这里不能使用console_put_str，因为还没有初始化
    put_str("I am kernel.\n");
    init_all();

    // thread_start("k_thread_a", 31, k_thread_function_a, "threadA :");
    // thread_start("k_thread_b", 31, k_thread_function_b, "threadB :");

    intr_enable();

    while (1) {
    }

    return 0;
}

void k_thread_function_a(void* args) {
    while(1)
    {
        enum intr_status old_satus = intr_disable();
        if(false == ioq_empty(&kbd_buf))
        {
            console_put_str(args);
            char byte = ioq_getchar(&kbd_buf);
            console_put_char(byte);
        }
        intr_set_status(old_satus);
    }
}

void k_thread_function_b(void* args) {
    while(1)
    {
        enum intr_status old_satus = intr_disable();
        if(false == ioq_empty(&kbd_buf))
        {
            console_put_str(args);
            char byte = ioq_getchar(&kbd_buf);
            console_put_char(byte);
        }
        intr_set_status(old_satus);
    }
}