#ifndef __FW_ASSIST_H__
#define __FW_ASSIST_H__

#include <string>
#include <map>

class FWItem;
class FWAssist
{
    typedef std::map<std::string, FWItem*> FWMap;
public:
    enum FWType
    {
        FW_Unknow = -1,
        FW_Flight,
        FW_FMU,
        FW_IMU,
    };
    enum FWStat
    {
        Stat_Uploading,
        Stat_Uploaded,
        Stat_UploadError,
    };
public:
    static FWAssist &Instance();
    static FWStat ProcessFW(const std::string &name, const void *buf, unsigned len, unsigned offset = 0, int tp = FW_Unknow, int size = 0);
    static int GetFw(const std::string &name, void *buf, int len, unsigned offset, unsigned *sz = 0);
    static std::string LastFwName(FWType tp = FW_Flight);
    static FWType GetFWType(const std::string &name);
    static int GetFWLength(const std::string &name);
    static uint32_t GetFWCrc32(const std::string &name);
    static void SetFWCrc32(const std::string &name, uint32_t crc);
    static bool GetFWRelease(const std::string &name);
    static void SetFWRelease(const std::string &name, bool release);
protected:
    FWAssist();
    ~FWAssist();
    FWItem *getFWItem(const std::string &name, FWType tp=FW_Unknow, unsigned sz=0);
private:
    FWMap m_fws;
};


#endif // __FW_ASSIST_H__