#include "FWItem.h"
#include "Utility.h"

FWItem::FWItem(unsigned sz, int tp, const std::string &name)
: m_timeUpload(Utility::msTimeTick()), m_lenFw(sz), m_fillFw(0)
, m_crc32(0), m_type(tp), m_dataFw(NULL)
{
    creatFw(name);
}

FWItem::~FWItem()
{
}

bool FWItem::AddData(const void* dt, int len)
{
    return false;
}

int FWItem::CopyData(void *dt, int len, int offset)
{
    int remind = m_lenFw - offset;

    return len < remind ? len : remind;
}

bool FWItem::IsValid() const
{
    return m_dataFw != NULL && m_lenFw > 0;
}

unsigned FWItem::GetFilled() const
{
    return m_fillFw;
}

unsigned FWItem::GetFWSize() const
{
    return m_lenFw;
}

int64_t FWItem::UploadTime() const
{
    return m_timeUpload;
}

int FWItem::GetType() const
{
    return m_type;
}

void FWItem::SetCrc32(unsigned crc)
{
    m_crc32 = crc;
}

unsigned FWItem::GetCrc32() const
{
    return m_crc32;
}

void FWItem::creatFw(const std::string &name)
{
}
