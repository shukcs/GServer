#include "FWItem.h"
#include "Utility.h"

#include "GSManager.h"
#include "VGMysql.h"
#include "VGDBManager.h"
#include "DBExecItem.h"
#include "ObjectGS.h"
#if defined _WIN32 || defined _WIN64
#include <Windows.h>
#else
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

FWItem::FWItem(int tp, unsigned sz, const std::string &name)
: m_timeUpload(Utility::msTimeTick()), m_lenFw(sz), m_fillFw(0)
, m_crc32(0), m_type(tp), m_dataFw(NULL)
{
    creatFw(name);
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
        memcpy(m_dataFw + m_fillFw, dt, len);
        m_fillFw += len;
#if !defined _WIN32 && !defined _WIN64
        if (m_fillFw == m_lenFw)
            msync(m_dataFw, m_fillFw, MS_SYNC);
#endif
        return true;
    }
    return false;
}

int FWItem::CopyData(void *dt, int len, int offset)
{
    int remind = (int)m_lenFw - offset;
    if (remind < 0 || !m_dataFw)
        return -1;
    if (len > remind)
        len = remind;

    if (len > 0)
        memcpy(dt, m_dataFw + offset, len);

    return len;
}

bool FWItem::IsValid() const
{
    return m_dataFw != NULL && m_lenFw > 0;
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

void FWItem::SetUploadTime(int64_t tm)
{
    m_timeUpload = tm;
}

int FWItem::GetType() const
{
    return m_type;
}

void FWItem::SetCrc32(unsigned crc)
{
    m_crc32 = crc;
}

unsigned FWItem::GetCrc32() const
{
    return m_crc32;
}

bool FWItem::LoadFW(const std::string &name)
{
    creatFw(name.c_str());
    if(m_dataFw)
        m_fillFw = m_lenFw;

    return m_dataFw != NULL;
}

void FWItem::creatFw(const std::string &name)
{
    if (name.empty())
        return;

    if (m_lenFw == 0)
    {
        struct stat temp;
        stat(name.c_str(), &temp);
        m_lenFw = (uint32_t)temp.st_size;
    }
#if defined _WIN32 || defined _WIN64
    m_hFile = CreateFileA(name.c_str(), GENERIC_READ|GENERIC_WRITE
        , FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_ALWAYS
        , FILE_ATTRIBUTE_NORMAL, NULL);
    if (m_hFile == INVALID_HANDLE_VALUE)
        return;

    m_hMap = CreateFileMapping(m_hFile, NULL, PAGE_READWRITE, 0, m_lenFw, NULL);
    if (!m_hMap)
    {
        CloseHandle(m_hFile);
        return;
    }

    m_dataFw = (char *)MapViewOfFile(m_hMap, FILE_MAP_ALL_ACCESS, 0, 0, m_lenFw);
    if (!m_dataFw)
    {
        printf("MapViewOfFile : %u\n", GetLastError());
        CloseHandle(m_hMap);
        m_hMap = NULL;
        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
        return;
    }
#else
    int fd = open(name.c_str(), O_RDWR);
    if (fd == -1)   //file not exist
    {
        fd = open(name.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        if (fd == -1)
        {
            printf("open file '%s' fail(%s)!\n", name.c_str(), strerror(errno));
            return;
        }
        static const char *sFill = "";
        lseek(fd, m_lenFw-1, SEEK_SET);
        write(fd, sFill, 1);    //fill empty file, if not mmap() fail;
    }

    m_dataFw = (char*)mmap(NULL, m_lenFw, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (!m_dataFw || m_dataFw==MAP_FAILED)
    {
        printf("mmap() fail(%s)!\n", strerror(errno));
        m_dataFw = NULL;
        return;
    }
#endif
}

VGMySql *FWItem::getSqlMgr()
{
    IObjectManager *mgr = IObject::GetManagerByType(ObjectGS::GSType());
    if (GSManager *gsm = dynamic_cast<GSManager *>(mgr))
        return gsm->GetMySql();

    return NULL;
}
