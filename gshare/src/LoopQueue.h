#ifndef __LOOP_QUEUE_H__
#define __LOOP_QUEUE_H__

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
    volatile uint16_t   m_pos[2];
};

class SHARED_DECL LoopQueueAbs
{
public:
    LoopQueueAbs();
    virtual ~LoopQueueAbs();

    void InitBuff(uint16_t sz);
    void *PushOne(const void *data, bool b = true);
    void *PopOne();
    bool IsValid()const;
    int BuffSize()const;
protected:
    virtual int getElementSize()const = 0;
    bool empty()const;
protected:
    DataNode    *m_dataPop;
    DataNode    *m_dataPush;
};

template <class EC>
class LoopQueue : protected LoopQueueAbs
{
public:
    LoopQueue() : LoopQueueAbs()
    {
    }
    bool Push(const EC &d)
    {
        if (!m_dataPush)
            InitBuff(32);

        if (void *men = PushOne(&d, true))
        {
            defaultConstruction((EC*)men, d);
            return true;
        }
        return false;
    }
    EC Pop()
    {
        if(void *men = PopOne())
        {
            EC ret(*(EC *)men);
            defaultDestruction((EC*)men);
            return ret;
        }
        if (TypeInfo<EC>::isPointer)
            return NULL;

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
        if (TypeInfo<EC>::isComplex)
            t->~EC();
    }
    int getElementSize()const
    {
        return sizeof(EC);
    }
};

#endif //__LOOP_QUEUE_H__
