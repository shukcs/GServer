#ifndef __FW_ITEM_H__
#define __FW_ITEM_H__

#include <string>

class FWItem
{
public:
    FWItem(unsigned sz, int tp, const std::string &name=std::string());
    virtual ~FWItem();

    bool AddData(const void* dt, int len);
    int CopyData(void* dt, int len, int offset);
    bool IsValid()const;
    unsigned GetFilled()const;
    unsigned GetFWSize()const;
    int64_t UploadTime()const;
    int GetType()const;
    void SetCrc32(unsigned crc);
    unsigned GetCrc32()const;
protected:
    void creatFw(const std::string &name);
private:
    int64_t     m_timeUpload;
    unsigned    m_lenFw;
    unsigned    m_fillFw;
    unsigned    m_crc32;
    int         m_type;
    char        *m_dataFw;
};


#endif // __FW_ASSIST_H__