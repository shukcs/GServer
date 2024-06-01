#include "LoopQueue.h"
#include <string.h>

#if defined _DEBUG
#define Warning(warn, fmt, ...) \
if(warn)\
    printf(fmt, ##__VA_ARGS__);
#else
#define Warning(warn, fmt, ...)
#endif
//////////////////////////////////////////////////////////////////////////
//LoopQueueAbs
//////////////////////////////////////////////////////////////////////////
LoopQueBuff::LoopQueBuff(uint32_t sz) : m_buff(nullptr), m_sizeBuff(0)
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
    if (nullptr == m_buff || m_sizeBuff < 1)
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
    return m_buff != nullptr && m_sizeBuff > 1;
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