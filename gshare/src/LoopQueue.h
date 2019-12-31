#ifndef __LOOP_QUEUE_H__
#define __LOOP_QUEUE_H__

/********************************************************************************
*多线程的循环队列，解决生产者/消费者之间不用加锁
*********************************************************************************/
#include <Varient.h>

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

class SHARED_DECL LoopQueueAbs
{
public:
    LoopQueueAbs();
    virtual ~LoopQueueAbs();

    void *PushOne(const void *data);
    void *CurrentBuff()const;
    void PopFinish();
protected:
    virtual int getElementSize()const = 0;
    bool empty()const;
    DataNode *recyclePop();
protected:
    DataNode    *m_dataRoot;
    DataNode    *m_dataPush;
    DataNode    *m_dataPops;
    DataNode    *m_popLast;
};

template <class EC>
class LoopQueue : protected LoopQueueAbs
{
public:
    LoopQueue() : LoopQueueAbs()
    {
    }
    ~LoopQueue()
    {
        while (!empty())
        {
            defaultDestruction((EC*)CurrentBuff());
            PopFinish();
        }
    }
    bool Push(const EC &d)
    {
        if (void *men = PushOne(&d))
        {
            defaultConstruction((EC*)men, d);
            return true;
        }
        return false;
    }
    EC Pop()
    {
        if(void *men = CurrentBuff())
        {
            EC ret(*(EC *)men);
            defaultDestruction((EC*)men);
            PopFinish();
            return ret;
        }

        return EC();
    }
    bool IsEmpty()const
    {
        return empty();
    }
protected:
    static void defaultConstruction(EC *te, const EC &d)
    {
        if (TypeInfo<EC>::isComplex)
            new (te) EC(d);
    }
    static void defaultDestruction(EC *t)
    {
        if (t && TypeInfo<EC>::isComplex)
            t->~EC();
    }
    int getElementSize()const
    {
        return sizeof(EC);
    }
};

#endif //__LOOP_QUEUE_H__
