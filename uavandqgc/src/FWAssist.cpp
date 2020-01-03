#include "FWAssist.h"
#include "FWItem.h"
#include "Utility.h"

using namespace std;
/////////////////////////////////////////////////////////
//FWAssist
/////////////////////////////////////////////////////////
FWAssist::FWAssist()
{
}

FWAssist::~FWAssist()
{
}

FWAssist &FWAssist::Instance()
{
    static FWAssist sAssist;
    return sAssist;
}

FWAssist::FWStat FWAssist::ProcessFW(const string &name, const void *buf, unsigned len, unsigned offset, int tp, int sz)
{
    FWItem *item = getFWItem(name, (FWType)tp, sz);
    FWStat ret = (item && item->GetFilled() == offset) ? Stat_Uploading : Stat_UploadError;
    if (ret!=Stat_UploadError)
    {
        item->AddData(buf, len);
        ret = (FWStat)item->CheckUploaded();
        if (Stat_UploadError == ret)
        {
            delete item;
            m_fws.erase(name);
        }
    }

    return ret;
}

int FWAssist::GetFw(const std::string &name, void *buf, int len, unsigned offset, unsigned *sz)
{
    if (FWItem *item = getFWItem(name))
    {
        if (sz)
            *sz = item->GetFWSize();
        if (buf)
            return item->CopyData(buf, len, offset);
    }
    return -1;
}

string FWAssist::LastFwName(FWType tp)const
{
    if (tp<FW_Flight && tp>FW_IMU)
        return string();

    const pair<string, FWItem*> *last = NULL;
    for (const pair<string, FWItem*> &itr : m_fws)
    {
        if (itr.second->GetType() == tp)
        {
            if (!last || last->second->UploadTime() > itr.second->UploadTime())
                last = &itr;
        }
    }

    return last? last->first : string();
}

FWAssist::FWType FWAssist::GetFWType(const string &name) const
{
    FWMap::const_iterator itr = m_fws.find(name);
    if (itr != m_fws.end())
        return (FWType)itr->second->GetType();

    return FW_Unknow;
}

int FWAssist::GetFWLength(const string &name) const
{
    FWMap::const_iterator itr = m_fws.find(name);
    if (itr != m_fws.end())
        return itr->second->GetFWSize();

    return 0;
}

uint32_t FWAssist::GetFWCrc32(const string &name) const
{
    FWMap::const_iterator itr = m_fws.find(name);
    if (itr != m_fws.end())
        return (FWType)itr->second->GetCrc32();

    return 0;
}

FWItem *FWAssist::getFWItem(const string &name, FWType tp, unsigned sz)
{
    if (name.empty())
        return NULL;

    FWMap::iterator itr = m_fws.find(name);
    if (itr != m_fws.end())
        return itr->second;

    if (sz > 0 && tp > FWAssist::FW_Unknow && tp <= FWAssist::FW_IMU)
    {
        FWItem *item = new FWItem(tp, sz, name);
        if (item)
            m_fws[name] = item;

        return item;
    }

    return NULL;
}