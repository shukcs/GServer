#ifndef __LOOP_QUEUE_H__
#define __LOOP_QUEUE_H__

#include <Utility.h>

class DataNode;
class SHARED_DECL LoopQueueAbs
{
public:
    LoopQueueAbs();
    virtual ~LoopQueueAbs();

    int Count()const;
    int CopyData(void *data, int len)const;
    void InitBuff(uint16_t sz);
    void PushOne(void *data, bool b = true);
    int Push(const void *data, uint16_t sz, bool removeOld = false);
    void *Pop();
    void Clear(uint16_t i);
    bool IsValid()const;
    int BuffSize()const;
    bool IsChanged()const;
    void Clear();
protected:
    virtual int getElementSize()const = 0;
    void releaseEmpty();
protected:
    DataNode    *m_data;
    bool        m_bChanged;
};

class SHARED_DECL LoopQueBuff : public LoopQueueAbs
{
public:
    LoopQueBuff(uint16_t sz = 0);

    bool ReSize(uint16_t sz);
protected:
    int getElementSize()const;
};

template <class EC>
class LoopQueue:public LoopQueueAbs
{
public:
    LoopQueue() : LoopQueueAbs(sizeof(EC))
    {
    }
    void defaultConstruct(EC *t)
    {
        if (TypeInfo<EC>::isComplex)
            new (t)EC();
    }
    void defaultDestruct(EC *t)
    {
        if (TypeInfo<EC>::isComplex)
            t->~EC();
    }
protected:
};

#endif //__LOOP_QUEUE_H__
