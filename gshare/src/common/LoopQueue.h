#ifndef __LOOP_QUEUE_H__
#define __LOOP_QUEUE_H__

#include <assert.h>
#include <stdint.h>
#include "stdconfig.h"

/********************************************************************************
*多线程的循环队列，解决生产者/消费者之间不用加锁
*********************************************************************************/
class DataNode;
class SHARED_DECL LoopQueBuff
{
public:
    LoopQueBuff(uint32_t sz = 0);
    virtual ~LoopQueBuff();

    bool ReSize(uint32_t sz);
    int Count()const;
    bool Push(const void *data, uint16_t sz, bool removeOld = false);
    int CopyData(void *data, int len)const;
    void Clear(int i = -1);
    bool IsValid()const;
    int BuffSize()const;
private:
    int getRemained()const;
    void remainLast(uint32_t remain);
private:
    char                *m_buff;
    int                 m_sizeBuff;

    //按道理不需要使用内存变量，安全起见
    volatile uint16_t   m_pos[2];
};

#undef USINGORIGINNEW
#endif //__LOOP_QUEUE_H__
