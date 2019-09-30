#include "FWAssist.h"
#include "Utility.h"

using namespace std;
/////////////////////////////////////////////////////////
//FWAssist
/////////////////////////////////////////////////////////
class FWItem {
public:
    FWItem(unsigned sz, int tp):m_lenFw(sz), m_fillFw(0)
        ,m_timeUpload(Utility::msTimeTick()),m_dataFw(NULL)
        ,m_type(tp)
    {
        m_dataFw = new char[sz];
        if (!m_dataFw)
            m_lenFw = 0;
    }
private:
    unsigned    m_lenFw;
    unsigned    m_fillFw;
    int64_t     m_timeUpload;
    char        *m_dataFw;
    int         m_type;
};
/////////////////////////////////////////////////////////
//FWAssist
/////////////////////////////////////////////////////////
FWAssist::FWAssist()
{
}

FWAssist::~FWAssist()
{
}

void FWAssist::CreateFW(const string &name, int tp, int size)
{

}

int FWAssist::CopyFW(void *buf, unsigned len, const string &name, int offset/*=0*/) const
{
    return len;
}

bool FWAssist::AddFWData(const string &name, const void *buf, unsigned len)
{
    return true;
}

FWItem *FWAssist::getFWItem(const string &name) const
{
    map<string, FWItem*>::const_iterator itr = m_fws.find(name);
    if (itr != m_fws.end())
        return itr->second;

    return NULL;
}
