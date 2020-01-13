#ifndef __FW_ITEM_H__
#define __FW_ITEM_H__

#include <string>

class VGMySql;
class FWItem
{
public:
    FWItem(const std::string &name, unsigned sz=0, int tp=-1);
    virtual ~FWItem();

    bool AddData(const void* dt, int len);
    int CheckUploaded();
    int CopyData(void* dt, int len, int offset);
    bool IsValid()const;
    bool IsRelease()const;
    void SetRelease(bool b);
    unsigned GetFilled()const;
    unsigned GetFWSize()const;
    int64_t UploadTime()const;
    int GetType()const;
    unsigned GetCrc32()const;
    void SetCrc32(unsigned crc);
public:
    static FWItem *LoadFW(const std::string &name);
protected:
    void creatFw(const std::string &name);
private:
    int64_t     m_timeUpload;
    unsigned    m_lenFw;
    unsigned    m_fillFw;
    unsigned    m_crc32;
    int         m_type;
    char        *m_dataFw;
    bool        m_bRelease;

#if defined _WIN32 || defined _WIN64
    void        *m_hFile;
    void        *m_hMap;
#endif
};


#endif // __FW_ASSIST_H__