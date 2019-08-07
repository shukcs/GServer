#include "BaseBuff.h"
#include <string.h>

BaseBuff::BaseBuff(uint16_t sz): m_buff(NULL), m_bChanged(false)
{
    m_lens[0] = 0;
    m_lens[1] = 0;
    InitBuff(sz);
}

BaseBuff::~BaseBuff()
{
    delete m_buff;
}

bool BaseBuff::InitBuff(uint16_t sz)
{
    uint8_t *buff = NULL;
    if (sz > m_lens[0])
    {
        buff = m_buff;
        if (m_buff = new uint8_t[sz])
        {
            memset(m_buff, 0, sz);
            m_lens[0] = sz;
            if (m_lens[1] > 0)
                memcpy(m_buff, buff, m_lens[1]);
        }
    }
    delete buff;
    return m_buff != NULL;
}

bool BaseBuff::AddOne(uint8_t val)
{
    if (m_buff && m_lens[1] < m_lens[0])
    {
        m_buff[m_lens[1]++]=val;
        m_lens[1]++;
        m_bChanged = true;
        return true;
    }

    return false;
}

bool BaseBuff::Add(const void *buff, uint16_t sz)
{
    if (buff && sz > 0 && m_buff && m_lens[1]+sz<=m_lens[0])
    {
        memcpy(m_buff + m_lens[1], buff, sz);
        m_lens[1] += sz;
        m_bChanged = true;
        return true;
    }
    return false;
}

void *BaseBuff::GetBuffAddress() const
{
    return m_buff;
}

uint16_t BaseBuff::Count() const
{
    if (m_buff)
        return m_lens[1];

    return 0;
}

uint16_t BaseBuff::CountMax() const
{
    return m_lens[0];
}

uint16_t BaseBuff::ReMained() const
{
    if (m_buff)
        return m_lens[0] - m_lens[1];

    return 0;
}

bool BaseBuff::IsValid() const
{
    return m_buff != NULL && m_lens[0]>0;
}

void BaseBuff::Clear(uint16_t sz)
{
    SetNoChange();
    if(!m_buff || sz>=m_lens[1])
    {
        m_lens[1] = 0;
        return;
    }

    m_lens[1] -= sz;
    memcpy(m_buff, m_buff+sz, m_lens[1]);
}

void BaseBuff::SetNoChange()
{
    m_bChanged = false;
}

bool BaseBuff::IsChanged() const
{
    return m_bChanged;
}
