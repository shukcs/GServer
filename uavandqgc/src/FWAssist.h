#ifndef __FW_ASSIST_H__
#define __FW_ASSIST_H__

#include <string>
#include <map>

class FWItem;
class FWAssist
{
public:
    FWAssist();
    ~FWAssist();

    void CreateFW(const std::string &name, int tp, int size);
    int CopyFW(void *buf, unsigned len, const std::string &name, int offset=0)const;
    bool AddFWData(const std::string &name, const void *buf, unsigned len);
protected:
    FWItem *getFWItem(const std::string &name)const;
private:
    std::map<std::string, FWItem*> m_fws;
};


#endif // __FW_ASSIST_H__