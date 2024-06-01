#include "FWItem.h"
#include "common/Utility.h"
#include "common/Crc.h"

#include "GSManager.h"
#include "VGMysql.h"
#include "DBExecItem.h"
#include "ObjectGS.h"
#include <stdio.h>
#if defined _WIN32 || defined _WIN64
#include <Windows.h>
#else
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#endif
#include "FWAssist.h"

#define TailerSize 17
FWItem::FWItem(const std::string &name, unsigned sz, int tp) : m_timeUpload(Utility::msTimeTick())
, m_lenFw(sz), m_fillFw(0), m_crc32(0), m_type(tp), m_dataFw(NULL), m_bRelease(false)
{
    creatFw(name);
    memcpy(m_dataFw, &m_timeUpload, sizeof(m_timeUpload));
    memcpy(m_dataFw+8, &m_type, sizeof(m_type));
}

FWItem::~FWItem()
{
    if (m_dataFw)
    {
#if defined _WIN32 || defined _WIN64
        UnmapViewOfFile(m_dataFw);
        CloseHandle(m_hMap);
        CloseHandle(m_hFile);
#else
        munmap(m_dataFw, m_lenFw);
#endif
    }
}

bool FWItem::AddData(const void *dt, int len)
{
    if (m_dataFw && dt && len>0 && m_fillFw+len<=m_lenFw)
    {
        memcpy(m_dataFw+m_fillFw, dt, len);
        m_fillFw += len;
        return true;
    }
    return false;
}

int FWItem::CheckUploaded()
{
    if (m_fillFw == m_lenFw)
    {
        uint32_t crc = Crc::Crc32(m_dataFw, m_lenFw);
        int ret = crc == m_crc32 ? FWAssist::Stat_Uploaded : FWAssist::Stat_UploadError;
        if (FWAssist::Stat_Uploaded == ret)
        {
            memcpy(m_dataFw+m_fillFw+12, &crc, sizeof(crc));
            m_dataFw[m_fillFw+TailerSize-1] = m_bRelease ? 1 : 0;
#if !defined _WIN32 && !defined _WIN64
            if (m_fillFw == m_lenFw)
                msync(m_dataFw, m_fillFw, MS_SYNC);
#endif
        }
        return ret;
    }
    return FWAssist::Stat_Uploading;
}

int FWItem::CopyData(void *dt, int len, int offset)
{
    int remind = (int)m_lenFw - offset;
    if (remind < 0 || !m_dataFw)
        return -1;
    if (len > remind)
        len = remind;

    if (len > 0)
        memcpy(dt, m_dataFw+offset, len);

    return len;
}

bool FWItem::IsValid() const
{
    return m_dataFw != NULL && m_lenFw > 0;
}

bool FWItem::IsRelease() const
{
    return m_bRelease;
}

void FWItem::SetRelease(bool b)
{
    m_bRelease = b;
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

unsigned FWItem::GetCrc32() const
{
    return m_crc32;
}

void FWItem::SetCrc32(unsigned crc)
{
    m_crc32 = crc;
}

FWItem *FWItem::LoadFW(const std::string &name)
{
    FWItem *ret = new FWItem(name);
    ret->creatFw(name.c_str());
    char *data = ret->m_dataFw;
    if(data && ret->m_type<0 && ret->m_lenFw>TailerSize)
    {
        ret->m_fillFw = ret->m_lenFw;
        int tmp = ret->m_fillFw;
        memcpy(&ret->m_timeUpload, data+tmp, tmp);
        tmp += sizeof(ret->m_timeUpload);
        ret->m_type = *(int*)(data+tmp);
        tmp += sizeof(m_type);
        ret->m_crc32 = *(uint32_t*)(data + tmp);
        tmp += sizeof(ret->m_crc32);
        ret->m_bRelease = data[tmp]!=0;
    }
    else
    {
        delete ret;
        ret = NULL;
    }

    return ret;
}

void FWItem::creatFw(const std::string &name)
{
    if (name.empty())
        return;

    int len = m_lenFw + TailerSize;
    if (m_lenFw == 0)
    {
        struct stat temp;
        stat(name.c_str(), &temp);
        len = (uint32_t)temp.st_size;
        m_lenFw = len-TailerSize;
    }
#if defined _WIN32 || defined _WIN64
    m_hFile = CreateFileA(name.c_str(), GENERIC_READ|GENERIC_WRITE
        , FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_ALWAYS
        , FILE_ATTRIBUTE_NORMAL, NULL);
    if (m_hFile == INVALID_HANDLE_VALUE)
        return;

    m_hMap = CreateFileMapping(m_hFile, NULL, PAGE_READWRITE, 0, len, NULL);
    if (!m_hMap)
    {
        CloseHandle(m_hFile);
        return;
    }

    m_dataFw = (char *)MapViewOfFile(m_hMap, FILE_MAP_ALL_ACCESS, 0, 0, len);
    if (!m_dataFw)
    {
        printf("MapViewOfFile : %u\n", GetLastError());
        CloseHandle(m_hMap);
        m_hMap = NULL;
        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
    }
#else
    int fd = open(name.c_str(), O_RDWR);
    if (fd == -1)   //file not exist
    {
        fd = open(name.c_str(), O_RDWR|O_CREAT, S_IRUSR | S_IWUSR);
        if (fd != -1)
        {
            lseek(fd, len - 1, SEEK_SET);
            if (write(fd, "\0", 1) > 0)   //fill empty file, if not mmap() fail;
                return; 
        }
        printf("open file '%s' fail(%s)!\n", name.c_str(), strerror(errno));
    }

    m_dataFw = (char*)mmap(NULL, len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (!m_dataFw || m_dataFw==MAP_FAILED)
    {
        printf("mmap() fail(%s)!\n", strerror(errno));
        m_dataFw = NULL;
    }
#endif
}
