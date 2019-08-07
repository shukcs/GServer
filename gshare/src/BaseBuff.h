#ifndef __BASE_BUFF_H__
#define __BASE_BUFF_H__

#include <stdbool.h>
#include <stdint.h>

class BaseBuff {
public:
    BaseBuff(uint16_t sz=0);
    ~BaseBuff();

    bool InitBuff(uint16_t sz);
    bool AddOne(uint8_t val);
    bool Add(const void *buff, uint16_t sz);
    void *GetBuffAddress()const;
    uint16_t Count()const;
    uint16_t CountMax()const;
    uint16_t ReMained()const;
    bool IsValid()const;
    void Clear(uint16_t sz=0xffff);
    void SetNoChange();
    bool IsChanged()const;
private:
    uint8_t     *m_buff;
    uint16_t    m_lens[2];
    bool        m_bChanged;
};

#endif //__BASE_BUFF_H__
