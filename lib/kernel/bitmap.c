#include "bitmap.h"            //函数定义
// #include "global.h"            //global.h 不清楚
#include "string.h"            //memset函数要用
#include "interrupt.h"         
#include "print.h"             
#include "debug.h"

#define BITMAP_MASK 1

void bitmap_init(struct bitmap* btmp)
{
    memset(btmp->bits,0,btmp->btmp_bytes_len);
    return;
}

bool bitmap_scan_test(struct bitmap* btmp,uint32_t bit_idx)  //一个8位的数 bit_idx/8 找数组下标 %得索引下的具体位置
{
    uint32_t byte_idx = bit_idx/8;
    uint32_t byte_pos = bit_idx%8;
    return (btmp->bits[byte_idx] & (BITMAP_MASK << byte_pos));
}

/*
   这个函数写很多的原因 是因为刚开始先用一个字节的快速扫描
   看是否每个字节中存在 位为0的位置 然后紧接着再看连续位 挺有意思 自己先写写看
*/

int bitmap_scan(struct bitmap* btmp,uint32_t cnt)
{
    ASSERT(cnt >= 1);
    uint32_t first_find_idx = 0;
    //解释一下0xff 一共8位 0xff = 11111111b
    while(first_find_idx < btmp->btmp_bytes_len && btmp->bits[first_find_idx] == 0xff)
    	++first_find_idx;
    if(first_find_idx == btmp->btmp_bytes_len)	return -1;
    
    uint32_t find_pos = 0;
    while((btmp->bits[first_find_idx] & (BITMAP_MASK << find_pos)))
    	++find_pos;
    if(cnt == 1)	return find_pos + 8*first_find_idx;
    
    uint32_t ret_pos = find_pos + 8*first_find_idx + 1,tempcnt = 1,endpos = (btmp->btmp_bytes_len)*8;
    while(ret_pos < endpos)
    {
    	if(!bitmap_scan_test(btmp,ret_pos))	++tempcnt;
    	else tempcnt = 0;
    	if(tempcnt == cnt)
    	    return ret_pos - tempcnt + 1;
    	++ret_pos;
    }    
    return -1;	
}

void bitmap_set(struct bitmap* btmp,uint32_t bit_idx,int8_t value)
{
    ASSERT(value == 1 || value == 0);
    uint32_t byte_idx = bit_idx/8;
    uint32_t byte_pos = bit_idx%8;
    if(value)	btmp->bits[byte_idx] |=  (BITMAP_MASK << byte_pos);
    else	btmp->bits[byte_idx] &= ~(BITMAP_MASK << byte_pos);
    return;
}
