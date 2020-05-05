#include "LoopQueue.h"
#include <string.h>

#if defined _DEBUG
#define Warning(warn, fmt, ...) \
if(warn)\
    printf(fmt, ##__VA_ARGS__);
#else
#define Warning(warn, fmt, ...)
#endif
/////////////////////////////////////////////////////////////////////
//DataNode
/////////////////////////////////////////////////////////////////////
class DataNode
{
public:
    DataNode(uint16_t elSz);
    ~DataNode();

    bool IsValid() const;
    void *GetBuff()const;
    DataNode *AddNode(int elSz);
    DataNode *NextNode()const;
    void SetNextNode(DataNode *nd);
protected:
    void initNode(uint32_t elSz);
protected:
    char                *m_buff;
    DataNode            *m_next;
};

DataNode::DataNode(uint16_t elSz) : m_buff(NULL)
, m_next(NULL)
{
    initNode(elSz);
}

DataNode::~DataNode()
{
    delete m_buff;
}

bool DataNode::IsValid() const
{
    return m_buff != NULL;
}

void *DataNode::GetBuff()const
{
    return m_buff;
}

DataNode * DataNode::AddNode(int elSz)
{
    DataNode *tmp = new DataNode(elSz);
    if (!tmp->IsValid())
    {
        delete tmp;
        return NULL;
    }

    m_next = tmp;
    return tmp;
}

DataNode *DataNode::NextNode()const
{
    return m_next;
}

void DataNode::SetNextNode(DataNode *nd)
{
    m_next = nd;
}

void DataNode::initNode(uint32_t elSz)
{
    if (elSz)
        m_buff = new char[elSz];
}
//////////////////////////////////////////////////////////////////////////
//LoopQueueAbs
//////////////////////////////////////////////////////////////////////////
LoopQueBuff::LoopQueBuff(uint32_t sz) : m_buff(NULL), m_sizeBuff(0)
{
    m_pos[0] = 0;
    m_pos[1] = 0;
    ReSize(sz);
}

LoopQueBuff::~LoopQueBuff()
{
    delete m_buff;
}

bool LoopQueBuff::ReSize(uint32_t sz)
{
    if (sz <= (uint32_t)m_sizeBuff)
        return true;

    int cnt = Count();
    if (char *buf = new char[sz + 1])
    {
        CopyData(buf, sz);
        m_pos[0] = 0;
        m_pos[1] = cnt;
        m_sizeBuff = sz;
        delete m_buff;
        m_buff = buf;
        return true;
    }

    return false;
}

int LoopQueBuff::Count() const
{
    if (NULL == m_buff || m_sizeBuff < 1)
        return 0;

    if (m_pos[0] == m_pos[1])
        return 0;

    if (m_pos[1] > m_pos[0])
        return m_pos[1] - m_pos[0];

    return (m_sizeBuff+1-m_pos[0])+m_pos[1];
}

bool LoopQueBuff::Push(const void *data, uint16_t len, bool removeOld /*= false*/)
{
    if (!data || len < 1 || (getRemained() < len && !removeOld))
        return false;

    const char *dataC = (char*)data;
    if (len >= m_sizeBuff)
    {
        int tmp = m_sizeBuff;
        dataC += len - tmp;
        len = tmp;
    }
    if (removeOld)
        remainLast(m_sizeBuff - len);
    int n = m_sizeBuff + 1 - m_pos[1];
    if (m_pos[0] > m_pos[1])
        n = m_pos[0] - m_pos[1];

    if (n > len)
        n = len;

    if (n > 0)
    {
        memcpy(m_buff + m_pos[1], dataC, n);
        m_pos[1] += n;
        if (m_pos[1] > m_sizeBuff)
            m_pos[1] = 0;
    }
    if (len > n)
    {
        m_pos[1] = len - n;
        memcpy(m_buff, dataC + n, m_pos[1]);
    }

    return true;
}

int LoopQueBuff::CopyData(void *data, int count) const
{
    int ret = Count();
    if (ret < 1 || !data || count < 1)
        return 0;

    if (ret > count)
        ret = count;
    bool bCped = m_pos[1] > m_pos[0];
    int nTail = bCped ? m_pos[1] - m_pos[0] : m_sizeBuff + 1 - m_pos[0];
    if (nTail > ret)
        nTail = ret;
    memcpy(data, m_buff + m_pos[0], nTail);
    if (!bCped && nTail < ret)
        memcpy((char*)data + nTail, m_buff, ret - nTail);

    return ret;
}

void LoopQueBuff::Clear(int i)
{
    int sz = Count();
    if (i < 0 || i >= sz)
    {
        m_pos[0] = m_pos[1];
        return;
    }

    m_pos[0] = (m_pos[0]+i)% (m_sizeBuff+1);
}

bool LoopQueBuff::IsValid() const
{
    return m_buff != NULL && m_sizeBuff > 1;
}

int LoopQueBuff::BuffSize() const
{
    if (IsValid())
        return m_sizeBuff;

    return 0;
}

int LoopQueBuff::getRemained() const
{
    if (!IsValid())
        return 0;

    return m_sizeBuff - Count();
}

void LoopQueBuff::remainLast(uint32_t remain)
{
    int sz = Count();
    if ((uint32_t)sz <= remain)
        return;

    m_pos[0] = (m_pos[1] - remain) % m_sizeBuff;
}
////////////////////////////////////////////////////////////////////////////
//LoopQueueAbs
////////////////////////////////////////////////////////////////////////////
LoopQueueAbs::LoopQueueAbs() : m_dataRoot(new DataNode(0)), m_dataPush(NULL)
, m_dataPops(NULL), m_popLast(NULL)
{
}

LoopQueueAbs::~LoopQueueAbs()
{
    DataNode *nd = m_dataPops;
    while (nd)
    {
        DataNode *tmp = nd->NextNode();
        delete nd;
        nd = tmp;
    }
    nd = m_dataRoot;
    while (nd)
    {
        DataNode *tmp = nd->NextNode();
        delete nd;
        nd = tmp;
    }
}

void *LoopQueueAbs::PushOne(const void *data)
{
    if (!m_dataPush && m_dataRoot)
    {
        DataNode *cur = m_dataRoot->AddNode(getElementSize());
        if (!cur)
            return NULL;

        m_dataPush = cur;
    }
    if (m_dataPush && data)
    {
        memcpy(m_dataPush->GetBuff(), data, getElementSize());
        if (!m_dataPush->NextNode())
        {
            if (auto tmp = recyclePop())
            {
                m_dataPush->SetNextNode(tmp);
                m_dataPush = tmp;
            }
            else
            {
                m_dataPush = m_dataPush->AddNode(getElementSize());
            }
        }
        m_count++;
        return m_dataPush->GetBuff();
    }

    return NULL;
}

void *LoopQueueAbs::CurrentBuff(DataNode *nd)const
{
    if (!nd)
        nd = m_dataRoot;

    DataNode *tmp = nd ? nd->NextNode() : NULL;
    if (tmp && tmp!=m_dataPush)
        return tmp->GetBuff();

    return NULL;
}

void LoopQueueAbs::PopFinish()
{
    DataNode *tmp = m_dataRoot ? m_dataRoot->NextNode() : NULL;
    if (tmp && tmp != m_dataPush)
    {
        m_dataRoot->SetNextNode(tmp->NextNode());
        tmp->SetNextNode(NULL);
        if (!m_dataPops)
        {
            m_dataPops = tmp;
            m_popLast = tmp;
        }
        else
        {
            m_popLast->SetNextNode(tmp);
            m_popLast = tmp;
        }
        m_count--;
    }
}

bool LoopQueueAbs::empty() const
{
    return !m_dataRoot || m_dataPush == NULL || m_dataRoot->NextNode()==m_dataPush;
}

DataNode *LoopQueueAbs::recyclePop()
{
    if (!m_dataPops || m_dataPops==m_popLast || !m_dataPush)
        return NULL;

    DataNode *nd = m_dataPops;
    m_dataPops = m_dataPops->NextNode();
    nd->SetNextNode(NULL);
    return nd;
}

DataNode *LoopQueueAbs::nextNode(DataNode *nd)const
{
    DataNode *ret = nd ? nd->NextNode() : NULL;
    if (ret == m_dataPush)
        return NULL;

    return ret;
}
