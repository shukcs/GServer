#include "LoopQueue.h"
#include <string.h>

/////////////////////////////////////////////////////////////////////
//DataNode
/////////////////////////////////////////////////////////////////////
class DataNode
{
public:
    DataNode(uint16_t sz, uint16_t elSz = 1);
    DataNode(const DataNode &node);
    ~DataNode();

    bool IsValid()const;
    void *GetElement(uint32_t i);
    int GetLength()const;
    DataNode *NextNode()const;
    DataNode *AddNode();
    void *PushOne(void *data, bool bAdd = false);
    bool  Push(const void *data, int len, bool removeOld);
    void *PopOne(DataNode *root = NULL);
    DataNode *Clear(int i=-1, DataNode *rt=NULL);
    int CopyData(void *data, uint32_t count)const;
    int GetRemained()const;
    DataNode *GetHeaderNode();
    void Resize(uint32_t sz);
    int GetAllocSize()const;
protected:
    void initNode(uint32_t sz, uint32_t elSz);
    DataNode *getLastNode();
    DataNode *getPushNode();
    void remainLast(uint16_t n);
protected:
    char            *m_buff;
    DataNode        *m_next;
    uint16_t        m_pos[2];
    uint16_t        m_size[2];
};

DataNode::DataNode(uint16_t sz, uint16_t elSz):m_buff(NULL),m_next(NULL)
{
    initNode(sz, elSz);
}

DataNode::DataNode(const DataNode &oth):m_buff(NULL),m_next(NULL)
{
    initNode(oth.m_size[0], oth.m_size[1]);
    Push(oth.m_buff, oth.GetLength(), false);
}

DataNode::~DataNode()
{
    delete m_buff;
}

bool DataNode::IsValid() const
{
    return m_buff!=NULL;
}

void *DataNode::GetElement(uint32_t i)
{
    if (IsValid() && i < m_size[0])
        return m_buff + ((i+m_pos[0])%m_size[0])*m_size[1];

    return NULL;
}

int DataNode::GetLength() const
{
    if (!IsValid())
        return 0;

    if (m_pos[0] == m_pos[1])
        return 0;

    if (m_pos[1] > m_pos[0])
        return m_pos[1] - m_pos[0];

    return m_pos[1] + m_size[0] - m_pos[0];
}

DataNode *DataNode::NextNode() const
{
    if (IsValid())
        return m_next;

    return NULL;
}

DataNode *DataNode::AddNode()
{
    if (!IsValid())
        return NULL;

    DataNode *tmp = new DataNode(*this);
    if (!tmp->IsValid())
    {
        delete tmp;
        return NULL;
    }
    DataNode *last = getLastNode();
    last->m_next = tmp;
    tmp->m_next = this;
    return last;
}

void *DataNode::PushOne(void *data, bool bAdd)
{
    if (!data)
        return NULL;

    if (GetRemained() <= 0)
    {
        if (!bAdd)
            return false;
        else if (DataNode *node = getPushNode())
            return node->PushOne(data, false);
    }

    void *buff = m_buff + int(m_pos[1]++)*m_size[1];
    memcpy(buff, data, m_size[1]);
    m_pos[1] %= m_size[0];
    return buff;
}

bool DataNode::Push(const void *data, int len, bool removeOld)
{
    if (m_size[1] != 1|| !data || len<1 || (GetRemained()<len && !removeOld) )
        return false;

    const char *dataC = (char*)data;
    if (len >= m_size[0])
    {
        int tmp = m_size[0] - 1;
        dataC += len-tmp;
        len = tmp;
    }
    if (removeOld)
        remainLast(m_size[0]-len-1);
    int n = m_pos[0]<=m_pos[1] ? m_size[0]-m_pos[1] : m_pos[0]-m_pos[1];
    if (m_pos[0] > m_pos[1])
        n = m_size[0] - m_pos[1];

    if (n > len)
        n = len;

    if(n>1)
    {
        memcpy(m_buff + m_pos[1], dataC, n);
        m_pos[1] += n;
    }
    if (len > n)
    {
        m_pos[1] = len - n;
        memcpy(m_buff, dataC + n, m_pos[1]);
    }
    return true;
}

void *DataNode::PopOne(DataNode *root)
{
    if (GetLength() < 1)
    {
        if (!root)
            root = this;
        if (m_next && m_next != root)
            return m_next->PopOne(root);
        return NULL;
    }
    void *buff = m_buff + int(m_pos[0]++)*m_size[1];
    m_pos[0] %= m_size[0];

    return buff;
}

DataNode *DataNode::Clear(int i, DataNode *rt)
{
    if (0 == i)
        return this;

    int sz = GetLength();
    int clr = (i>0&&i<sz) ? i : sz;
    m_pos[0] = (m_pos[0]+clr) % m_size[0];
    DataNode *next = m_next;
    if (m_pos[0] == m_pos[1])
    {
        if (!rt && rt!=this)
            delete this;
    }
    if (i > 0)
        i -= clr;

    if (next && i!=0)
    {
        DataNode *tmp = next->Clear(i, rt);
        if (rt)
            return tmp;
    }

    return this;
}

int DataNode::CopyData(void *data, uint32_t count)const
{
    int ret = GetLength();
    if (m_size[1] != 1 || ret < 1 || !data || count < 1)
        return 0;

    if (ret > (int)count)
        ret = count;
    bool bCped = m_pos[1] > m_pos[0];
    int nTail = bCped ? m_pos[1]-m_pos[0]:m_size[0]-m_pos[0];
    if (nTail > ret)
        nTail = ret;
    memcpy(data, m_buff+m_pos[0], nTail);
    if (!bCped && nTail<ret)
        memcpy((char*)data+nTail, m_buff, ret-nTail);

    return ret;
}

int DataNode::GetRemained() const
{
    if (!IsValid())
        return 0;
    int ret = int(m_size[0]) - GetLength() - 1;
    return ret;
}

DataNode *DataNode::GetHeaderNode()
{
    if (GetLength() > 0)
        return this;
    if (!m_next)
        return NULL;

    return m_next->GetHeaderNode();
}

void DataNode::Resize(uint32_t sz)
{
    if (sz < m_size[0])
        return;

    int cnt = GetLength();
    if (char *buf = new char[sz * m_size[1]])
    {
        CopyData(buf, sz);
        m_pos[0] = 0;
        m_pos[1] = cnt;
        m_size[0] = sz;
        delete m_buff;
        m_buff = buf;
    }
}

int DataNode::GetAllocSize() const
{
    return IsValid() ? m_size[0] : 0;
}

void DataNode::initNode(uint32_t sz, uint32_t elSz)
{
    if (m_buff = new char[sz * elSz])
    {
        m_size[0] = sz;
        m_size[1] = elSz;
        m_pos[0] = 0;
        m_pos[1] = 0;
    }
}

DataNode *DataNode::getLastNode()
{
    DataNode *ret = this;
    while(ret->m_next && ret->m_next!=this)
    {
        ret = ret->m_next;
    }
    return ret;
}

DataNode *DataNode::getPushNode()
{
    DataNode *ret = this;
    while (GetRemained() < 0)
    {
        if (ret->m_next && ret->m_next != this)
            return ret->AddNode();
        ret = ret->m_next;
    }
    return ret;
}

void DataNode::remainLast(uint16_t n)
{
    int sz = GetLength();
    if (sz <= n)
        return;

    m_pos[0] = (m_pos[1] - n) % m_size[0];
}
//////////////////////////////////////////////////////////////////////////
//LoopQueueAbs
//////////////////////////////////////////////////////////////////////////
LoopQueueAbs::LoopQueueAbs() : m_data(NULL), m_bChanged(false)
{
}

LoopQueueAbs::~LoopQueueAbs()
{
    DataNode *nd = m_data;
    while (nd && nd != m_data)
    {
        DataNode *tmp = nd->NextNode();
        delete nd;
        nd = tmp;
    }
}

int LoopQueueAbs::Count() const
{
    return m_data ? m_data->GetLength() : 0;
}

int LoopQueueAbs::CopyData(void *data, int len)const
{
    if (m_data)
        return m_data->CopyData(data, len);

    return 0;
}

void LoopQueueAbs::InitBuff(uint16_t sz)
{
    if (sz < 1)
        return;

    if (!m_data)
        m_data = new DataNode(sz, getElementSize());
    else if (sz > m_data->GetAllocSize())
        m_data->Resize(sz);
}

void LoopQueueAbs::PushOne(void *data, bool b)
{
    if (m_data && m_data->PushOne(data, b))
        m_bChanged = true;
}

int LoopQueueAbs::Push(const void *data, uint16_t sz, bool removeOld)
{
    if (m_data && m_data->Push(data, sz, removeOld))
    {
        m_bChanged = true;
        return sz;
    }
    return 0;
}

void *LoopQueueAbs::Pop()
{
    void *ret = NULL;
    if (!m_data)
        ret = m_data->PopOne();
    m_bChanged = false;
    return ret;
}

void LoopQueueAbs::Clear(uint16_t i)
{
    m_bChanged = false;
    if (m_data && i!=0)
        m_data = m_data->Clear(i, m_data);
}

bool LoopQueueAbs::IsValid() const
{
    return m_data && m_data->IsValid();
}

int LoopQueueAbs::BuffSize() const
{
    if (m_data)
        return m_data->GetAllocSize()-1;

    return 0;
}

bool LoopQueueAbs::IsChanged() const
{
    return m_bChanged;
}

void LoopQueueAbs::Clear()
{
    DataNode *nd = m_data->NextNode();
    m_data->Clear();
    while (nd && nd != m_data)
    {
        DataNode *tmp = nd->NextNode();
        delete nd;
        nd = tmp;
    }
}

void LoopQueueAbs::releaseEmpty()
{
    while (m_data && m_data->GetLength()<1)
    {
        DataNode *tmp = m_data->NextNode();
        if (m_data == tmp)
            break;
        delete m_data;
        m_data = tmp;
    }
}
//////////////////////////////////////////////////////////////////////////
//LoopQueueAbs
//////////////////////////////////////////////////////////////////////////
LoopQueBuff::LoopQueBuff(uint16_t sz /*= 0*/):LoopQueueAbs()
{
    InitBuff(sz);
}

bool LoopQueBuff::ReSize(uint16_t sz)
{
    InitBuff(sz);
    return m_data->IsValid();
}

int LoopQueBuff::getElementSize() const
{
    return 1;
}
