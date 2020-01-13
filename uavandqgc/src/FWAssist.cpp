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
    FWAssist &fa = Instance();
    FWItem *item = fa.getFWItem(name, (FWType)tp, sz);
    FWStat ret = (item && item->GetFilled() == offset) ? Stat_Uploading : Stat_UploadError;
    if (ret!=Stat_UploadError)
    {
        item->AddData(buf, len);
        ret = (FWStat)item->CheckUploaded();
        if (Stat_UploadError == ret)
        {
            delete item;
            fa.m_fws.erase(name);
        }
    }

    return ret;
}

int FWAssist::GetFw(const std::string &name, void *buf, int len, unsigned offset, unsigned *sz)
{
    FWAssist &fa = Instance();
    if (FWItem *item = fa.getFWItem(name))
    {
        if (sz)
            *sz = item->GetFWSize();
        if (buf)
            return item->CopyData(buf, len, offset);
    }
    return -1;
}

string FWAssist::LastFwName(FWType tp)
{
    if (tp<FW_Flight && tp>FW_IMU)
        return string();

    FWAssist &fa = Instance();
    const pair<string, FWItem*> *last = NULL;
    for (const pair<string, FWItem*> &itr : fa.m_fws)
    {
        if (itr.second->GetType() == tp)
        {
            if (!last || last->second->UploadTime() > itr.second->UploadTime())
                last = &itr;
        }
    }

    return last? last->first : string();
}

FWAssist::FWType FWAssist::GetFWType(const string &name)
{
    FWAssist &fa = Instance();
    FWMap::const_iterator itr = fa.m_fws.find(name);
    if (itr != fa.m_fws.end())
        return (FWType)itr->second->GetType();

    return FW_Unknow;
}

int FWAssist::GetFWLength(const string &name)
{
    FWAssist &fa = Instance();
    FWMap::const_iterator itr = fa.m_fws.find(name);
    if (itr != fa.m_fws.end())
        return itr->second->GetFWSize();

    return 0;
}

uint32_t FWAssist::GetFWCrc32(const string &name)
{
    FWAssist &fa = Instance();
    FWMap::const_iterator itr = fa.m_fws.find(name);
    if (itr != fa.m_fws.end())
        return (FWType)itr->second->GetCrc32();

    return 0;
}

void FWAssist::SetFWCrc32(const std::string &name, uint32_t crc)
{
    FWAssist &fa = Instance();
    FWMap::iterator itr = fa.m_fws.find(name);
    if (itr != fa.m_fws.end())
        itr->second->SetCrc32(crc);
}

bool FWAssist::GetFWRelease(const std::string &name)
{
    FWAssist &fa = Instance();
    FWMap::const_iterator itr = fa.m_fws.find(name);
    if (itr != fa.m_fws.end())
        return (FWType)itr->second->IsRelease();

    return false;
}

void FWAssist::SetFWRelease(const std::string &name, bool release)
{
    FWAssist &fa = Instance();
    FWMap::iterator itr = fa.m_fws.find(name);
    if (itr != fa.m_fws.end())
        itr->second->SetRelease(release);
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
        FWItem *item = new FWItem(name, sz, tp);
        if (item)
            m_fws[name] = item;

        return item;
    }

    return NULL;
}