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
    DataNode(uint16_t sz, uint16_t elSz = 1);
    ~DataNode();

    bool IsValid()const;
    void *GetElement(uint32_t i);
    int GetLength()const;
    bool IsEmpty()const;
    DataNode *NextNode(bool bAdd=false);
    void *PushOne(const void *data);
    void *PopOne(DataNode *root = NULL);
    DataNode *Clear(int i=-1, DataNode *rt=NULL);
    int GetRemained()const;
    int GetAllocSize()const;
protected:
    void initNode(uint32_t sz, uint32_t elSz);
    void remainLast(uint16_t n);
    DataNode *genNewEmpty();
    DataNode *addNode(DataNode *root = NULL);
protected:
    char            *m_buff;
    DataNode        *m_next;
    bool            m_bLastEmpty;
    uint16_t        m_pos[2];
    uint16_t        m_size[2];
};

DataNode::DataNode(uint16_t sz, uint16_t elSz)
:m_buff(NULL),m_next(NULL), m_bLastEmpty(true)
{
    initNode(sz, elSz);
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

    if (m_pos[0]==m_pos[1] && m_bLastEmpty)
        return 0;

    if (m_pos[1] > m_pos[0])
        return m_pos[1] - m_pos[0];

    return m_size[0] - m_pos[0] + m_pos[1];
}

bool DataNode::IsEmpty() const
{
    return m_pos[0] == m_pos[1] && m_bLastEmpty;
}

DataNode *DataNode::NextNode(bool bAdd)
{
    if (IsValid())
    {
        if(!bAdd || m_next)
            return m_next;

        return addNode();
    }

    return NULL;
}

DataNode *DataNode::addNode(DataNode *root)
{
    if (!IsValid())
        return NULL;

    if (!root)
        root = this;

    DataNode *tmp = genNewEmpty();
    if (!tmp->IsValid())
    {
        delete tmp;
        return NULL;
    }
    tmp->m_next = m_next ? m_next : root;
    m_next = tmp;
    return tmp;
}

void *DataNode::PushOne(const void *data)
{
    if (!data || GetRemained() <= 0)
        return NULL;

    void *buff = m_buff + int(m_pos[1])*m_size[1];
    memcpy(buff, data, m_size[1]);
    if (m_pos[1] + 1 >= m_size[0])
        m_pos[1] = 0;
    else
        ++m_pos[1];
    if (m_pos[1] == m_pos[0])
        m_bLastEmpty = false;
    return buff;
}

void *DataNode::PopOne(DataNode *root)
{
    if (IsEmpty())
        return NULL;

    char *buff = m_buff + int(m_pos[0])*m_size[1];
    if (m_pos[0] + 1 >= m_size[0])
        m_pos[0] = 0;
    else
        ++m_pos[0];
    if (!m_bLastEmpty)
        m_bLastEmpty = true;

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

int DataNode::GetRemained() const
{
    if (!IsValid())
        return 0;
    return GetAllocSize() - GetLength();
}

int DataNode::GetAllocSize() const
{
    return IsValid() ? m_size[0] : 0;
}

void DataNode::initNode(uint32_t sz, uint32_t elSz)
{
    m_buff = new char[sz * elSz];
    if (m_buff != NULL)
    {
        m_size[0] = sz;
        m_size[1] = elSz;
        m_pos[0] = 0;
        m_pos[1] = 0;
    }
}

void DataNode::remainLast(uint16_t n)
{
    int sz = GetLength();
    if (sz <= n)
        return;

    m_pos[0] = (m_pos[1] - n) % m_size[0];
}

DataNode *DataNode::genNewEmpty()
{
    return new DataNode(m_size[0], m_size[1]);
}
//////////////////////////////////////////////////////////////////////////
//LoopQueueAbs
//////////////////////////////////////////////////////////////////////////
LoopQueueAbs::LoopQueueAbs() : m_dataPush(NULL), m_dataPop(NULL)
{
}

LoopQueueAbs::~LoopQueueAbs()
{
    DataNode *nd = m_dataPop;
    while (nd && nd != m_dataPop)
    {
        DataNode *tmp = nd->NextNode();
        delete nd;
        nd = tmp;
    }
}

void LoopQueueAbs::InitBuff(uint16_t sz)
{
    if (sz < 1)
        return;

    if (!m_dataPush)
        m_dataPush = new DataNode(sz, getElementSize());

    if(!m_dataPop)
        m_dataPop = m_dataPush;
}

void *LoopQueueAbs::PushOne(const void *data, bool b)
{
    void *men = NULL;
    if (m_dataPush && data)
    {
        men = m_dataPush->PushOne(data);
        if (!men && b)
        {
            m_dataPush = m_dataPush->NextNode(true);
            men = m_dataPush->PushOne(data);
        }
    }
    return men;
}

void *LoopQueueAbs::PopOne()
{
    void *men = NULL;
    if (m_dataPop)
    {
        men = m_dataPop->PopOne();
        if (m_dataPop->IsEmpty() && m_dataPop != m_dataPush)
        {
            DataNode *next = m_dataPop->NextNode();
            if (next)
                m_dataPop = next;
        }
        Warning(men, "%s: failed!\r\n", __FUNCTION__);
    }

    return men;
}

bool LoopQueueAbs::IsValid() const
{
    return m_dataPush && m_dataPush->IsValid();
}

int LoopQueueAbs::BuffSize() const
{
    if (m_dataPop)
        return m_dataPop->GetAllocSize()-1;

    return 0;
}

void LoopQueueAbs::releaseEmpty()
{
    while (m_dataPop && m_dataPop->GetLength()<1)
    {
        DataNode *tmp = m_dataPop->NextNode();
        if (m_dataPop == tmp)
            break;
        delete m_dataPop;
        m_dataPop = tmp;
    }
}

bool LoopQueueAbs::empty() const
{
    return m_dataPop == NULL || m_dataPop->IsEmpty();
}
//////////////////////////////////////////////////////////////////////////
//LoopQueueAbs
//////////////////////////////////////////////////////////////////////////
LoopQueBuff::LoopQueBuff(uint32_t sz): m_buff(NULL),m_sizeBuff(0) 
, m_bLastEmpty(true)
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
    if (char *buf = new char[sz])
    {
        CopyData(buf, sz);
        m_bLastEmpty = true;
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
    if (NULL == m_buff || m_sizeBuff<1)
        return 0;

    if (m_pos[0] == m_pos[1] && m_bLastEmpty)
        return 0;

    if (m_pos[1] > m_pos[0])
        return m_pos[1] - m_pos[0];

    return m_sizeBuff - m_pos[0] + m_pos[1];
}

bool LoopQueBuff::Push(const void *data, uint16_t len, bool removeOld /*= false*/)
{
    if (!data || len < 1 || (getRemained()<len && !removeOld))
        return false;

    const char *dataC = (char*)data;
    if (len >= m_sizeBuff)
    {
        int tmp = m_sizeBuff - 1;
        dataC += len - tmp;
        len = tmp;
    }
    if (removeOld)
        remainLast(m_sizeBuff - len);
    int n = m_sizeBuff - m_pos[1];
    if (m_pos[0] > m_pos[1])
        n = m_pos[0] - m_pos[1];

    if (n > len)
        n = len;

    if (n > 0)
    {
        memcpy(m_buff + m_pos[1], dataC, n);
        m_pos[1] += n;
    }
    if (len > n)
    {
        m_pos[1] = len - n;
        memcpy(m_buff, dataC + n, m_pos[1]);
    }
    if (m_pos[1] == m_pos[0])
        m_bLastEmpty = false;
    return true;
}

int LoopQueBuff::CopyData(void *data, int count) const
{
    int ret = Count();
    if (ret < 1 || !data || count < 1)
        return 0;

    if (ret > (int)count)
        ret = count;
    bool bCped = m_pos[1] > m_pos[0];
    int nTail = bCped ? m_pos[1]-m_pos[0] : m_sizeBuff-m_pos[0];
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
    int clr = (i > 0 && i < sz) ? i : sz;
    m_pos[0] = (m_pos[0] + clr) % m_sizeBuff;
    if (!m_bLastEmpty)
        m_bLastEmpty = false;
}

bool LoopQueBuff::IsValid() const
{
    return m_buff!=NULL && m_sizeBuff > 1;
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
