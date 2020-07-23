#ifndef __LOOP_QUEUE_H__
#define __LOOP_QUEUE_H__

#define USINGORIGINNEW
#include "Varient.h"
#include <assert.h>

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

template <class EC>
class NodeItem
{
    typedef NodeItem<EC> Node;
public:
    NodeItem(const EC &val):m_val(val), m_next(NULL)
    {
    }
    const EC &GetValue()const
    {
        return m_val;
    }
    void SetValue(const EC &v)
    {
        m_val = v;
    }
    Node *GetNext()const
    {
        return m_next;
    }
    void SetNext(Node *nd)
    {
        m_next = nd;
    }
private:
    EC      m_val;
    Node    *m_next;
};

template <class EC>
class LoopQueue
{
    typedef NodeItem<EC> QueNode;
public:
    LoopQueue() : m_header(NULL), m_push(NULL), m_pop(NULL), m_count(0), m_nEmpty(0)
    {
    }
    ~LoopQueue()
    {
        auto node = m_header;
        while (node)
        {
            auto tmp = node->GetNext();
            delete node;
            node = tmp;
        }
    }
    bool Push(const EC &val, uint32_t max=20)
    {
        bool ret = setNext(val);
        if (!m_pop)
            m_pop = m_push;
        if (!m_header)
            m_header = m_push;

        removeMore(max);
        return ret;
    }
    EC Pop()
    {
        if (m_count <= 0)
            assert(m_pop);
        EC ret(m_pop->GetValue());
        m_pop = m_pop->GetNext();
        m_count--;
        m_nEmpty++;
        return ret;
    }
    bool IsEmpty()const
    {
        return m_pop==NULL || m_pop==m_push->GetNext();
    }
    bool IsContains(const EC &val)const
    {
        QueNode *nd = m_pop;
        while (nd)
        {
            if (nd->GetValue() == val)
                return true;

            nd = nd->GetNext();
        }
        return false;
    }
    int ElementCount()const
    {
        return m_count;
    }
    const EC &Last()const
    {
        assert(m_push);
        return m_push->GetValue();
    }
private:
    QueNode *takeEmpty()
    {
        if (m_nEmpty>0 && m_header && m_header->GetNext() && m_header!=m_pop && m_header!=m_push)
        {
            auto ret = m_header;
            m_header = m_header->GetNext();
            m_nEmpty--;
            ret->SetNext(NULL);
            return ret;
        }
        return NULL;
    }
    void removeMore(uint32_t max)
    {
        while (m_nEmpty > (int32_t)max)
        {
            if (auto tmp = takeEmpty())
                delete tmp;
            else
                break;
        }
    }
    bool genPush(const EC &val)
    {
        m_push = new QueNode(val);
        return m_push != NULL;
    }
    bool setNext(const EC &val)
    {
        bool ret = false;
        if (auto tmp = takeEmpty())
        {
            tmp->SetValue(val);
            m_push->SetNext(tmp);
            ++m_count;
            m_push = tmp;
            ret = true;
        }
        else if (auto tmp = new QueNode(val))
        {
            if (m_push)
                m_push->SetNext(tmp);
            ++m_count;
            m_push = tmp;
            ret = true;
        }

        return ret;
    }
private:
    QueNode             *m_header;
    QueNode             *m_push;
    QueNode             *m_pop;
    volatile int32_t    m_count;
    volatile int32_t    m_nEmpty;
};

#undef USINGORIGINNEW
#endif //__LOOP_QUEUE_H__
