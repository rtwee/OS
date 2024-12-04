#ifndef __DEVICE_IOQUEUE_H
#define __DEBICE_IOQUEUE_H
#include "stdint.h"
#include "thread.h"
#include "sync.h"

#define bufsize 64

//环型队列
struct ioqueue
{
    struct lock lock;
    //生产者，缓冲区不满的时候就往里面写数据，否则就睡眠，这里是表示睡眠的时候
    struct task_struct * producer;
    //消费者，缓冲区不空的时候就从里面拿出数据,否则就睡眠
    struct task_struct * consumer;
    char buf[bufsize];  //缓冲区大小
    int32_t head;   //队首
    int32_t tail;   //队尾

};

bool ioq_full(struct ioqueue * ioq);
void ioq_putchar(struct ioqueue * ioq,char byte);
char ioq_getchar(struct ioqueue * ioq);
void ioqueue_init(struct ioqueue * ioq);
bool ioq_empty(struct ioqueue * ioq);

#endif