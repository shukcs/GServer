#ifndef __FW_ASSIST_H__
#define __FW_ASSIST_H__

#include <string>
#include <map>

class FWItem;
class FWAssist
{
public:
    enum FWType
    {
        FW_Unknow = -1,
        FW_Flight,
        FW_FMU,
        FW_IMU,
    };
public:
    static FWAssist &Instance();
public:
    bool ProcessFW(const std::string &name, const void *buf, unsigned len, unsigned offset=0, int tp=FW_Unknow, int size=0);
    int GetFw(const std::string &name, void *buf, int len, unsigned offset, unsigned *sz = 0);
    std::string LastFwName(FWType tp = FW_Flight)const;
protected:
    FWAssist();
    ~FWAssist();
    FWItem *getFWItem(const std::string &name, FWType tp=FW_Unknow, unsigned sz=0);
private:
    std::map<std::string, FWItem*> m_fws;
};


#endif // __FW_ASSIST_H__