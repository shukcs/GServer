#include "processmanager.h"
#include <chrono>
#include <string.h>
#if defined _WIN32 || defined _WIN64
#include <winsock2.h>
#include <process.h>
#else
#include <unistd.h>
#define MAX_PATH 1024
#endif //defined _WIN32 || defined _WIN64

using namespace std;
typedef struct _processData {
    string  name;
    int     sock;
    char    buff[256];
    int     bufflen;
    bool    detected;
    bool    valid;
    int64_t lastTm;
} ProcessData;

static void ReplacePart(string &str, char part, char rpc)
{
    int count = str.length();
    char *ptr = &str.front();
    for (int pos = 0; pos <= count; ++pos)
    {
        if (ptr[pos] == part)
            ptr[pos] = rpc;
    }
}

static string ModulePath()
{
    char strRet[MAX_PATH] = { 0 };
#ifdef _WIN32
    DWORD ret = ::GetModuleFileNameA(NULL, strRet, MAX_PATH - 1);
#else
    size_t ret = readlink("/proc/self/exe", strRet, MAX_PATH - 1);
#endif
    if (ret < 1)
        string();

    string str = strRet;
    ReplacePart(str, '\\', '/');
    int idx = str.find_last_of('/');
    return idx >= 0 ? str.substr(0, idx + 1) : std::string();
}

StringList SplitString(const string &str, const string &sp, bool bSkipEmpty/*=false*/)
{
    StringList strLsRet;
    int nSizeSp = sp.size();
    if (!nSizeSp)
        return strLsRet;

    unsigned nPos = 0;
    while (nPos < str.size())
    {
        int nTmp = str.find(sp, nPos);
        string strTmp = str.substr(nPos, nTmp < 0 ? -1 : nTmp - nPos);
        if (strTmp.size() || !bSkipEmpty)
            strLsRet.push_back(strTmp);

        if (nTmp < int(nPos))
            break;
        nPos = nTmp + nSizeSp;
    }
    return strLsRet;
}

static void StartModule(const std::string &mod)
{
    auto lsStr = SplitString(mod, " ", true);
    if (lsStr.size() < 1)
        return;

    std::string path = ModulePath();
    auto itr = lsStr.begin();
    char *pp[4] = { NULL };
    char szLine[512] = { 0 };
    snprintf(szLine, sizeof(szLine), "%s%s", path.c_str(), lsStr.front().c_str());
    int i = 0;
    for (; itr != lsStr.end() && i<3; ++itr)
    {
        pp[i++] = &itr->front();
    }
#if !(defined _WIN32 || defined _WIN64)
    if (fork() == 0) //child or not using bBackground
    {
        execvp(szLine, pp);
        exit(0);
    }
#else
    spawnvp(_P_NOWAIT, szLine, pp);
#endif
}

static int FindString(const char *src, int len, const string &cnt)
{
    int count = cnt.length();
    if (count<1 || !src || len<count)
        return -1;

    for (int pos = 0; pos <= len - count; ++pos)
    {
        if (0 == strncmp(src, cnt.c_str(), (size_t)count))
            return pos;

        src++;
    }
    return -1;
}
/////////////////////////////////////////////////////////////////
//WSAInitializer
/////////////////////////////////////////////////////////////////
class WSAInitializer // Winsock Initializer
{
public:
    WSAInitializer()
    {
#if defined _WIN32 || defined _WIN64
        WSADATA wsadata;
        if (WSAStartup(MAKEWORD(2, 2), &wsadata))
        {
            exit(-1);
        }
#endif //defined _WIN32 || defined _WIN64
    }
    ~WSAInitializer()
    {
#if defined _WIN32 || defined _WIN64
        WSACleanup();
#endif //defined _WIN32 || defined _WIN64
    }
private:
};
//////////////////////////////////////////////////
//ProcessManager
//////////////////////////////////////////////////
ProcessManager::ProcessManager()
{
    static WSAInitializer sIn;
}

void ProcessManager::AddProcess(const std::string &p)
{
    for (const string &itr : m_processes)
    {
        if (p == itr)
            return;
    }

    m_processes.push_back(p);
    StartModule(p);
}

void ProcessManager::AddSocket(int fd)
{
    auto itr = m_sockets.begin();
    for (;itr != m_sockets.end(); ++itr)
    {
        if (fd == *itr)
            return;
        if (fd < *itr)
        {
            m_sockets.insert(itr, fd);
            return;
        }
    }
    m_sockets.push_back(fd);
}

void ProcessManager::RemoveSocket(int fd)
{
    m_sockets.remove(fd);
}

void ProcessManager::Receive(int sock, const char *buff, int len)
{
    if (sock < 1 || !buff || len < 1)
        return;

    auto p = GetProcess(sock);
    if (!p)
        p = GetInitProcess();

    if (p)
    {
        int cp = len > sizeof(p->buff) ? sizeof(p->buff) : len;
        memcpy(p->buff, buff+len-cp, cp);
        p->bufflen = cp;
        p->sock = sock;
    }
}

void ProcessManager::CheckProcess(int64_t ms)
{
    for (auto itr : m_processesData)
    {
        for (const string name : m_processes)
        {
            if (itr->bufflen >0 && FindString(itr->buff, itr->bufflen, name)>=0)
            {
                itr->bufflen = 0;
                itr->lastTm = ms;
                itr->detected = true;
                itr->name = name;
            }
        }
        if (ms - itr->lastTm > 3000 && !itr->name.empty())
        {
            itr->valid = false;
            itr->sock = -1;
            itr->bufflen = 0;
            itr->lastTm = ms;
            StartModule(itr->name);
        }
    }
}

const std::list<int> &ProcessManager::Sockets() const
{
    return m_sockets;
}

int ProcessManager::MaxFD() const
{
    if (m_sockets.size() > 0)
        return *(--m_sockets.end()) + 1;
    return 0;
}

int64_t ProcessManager::msTimeTick()
{
    chrono::system_clock::time_point now = chrono::system_clock::now();
    auto ms = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch());
    return ms.count();
}

ProcessData *ProcessManager::GetProcess(const std::string &p) const
{
    for (auto *itr : m_processesData)
    {
        if (p == itr->name)
            return itr;
    }
    return NULL;
}

ProcessData * ProcessManager::GetProcess(int sock) const
{
    for (auto *itr : m_processesData)
    {
        if (sock == itr->sock)
            return itr;
    }
    return NULL;
}

ProcessData * ProcessManager::GetInitProcess()
{
    for (auto itr : m_processesData)
    {
        if (!itr->valid)
        {
            itr->valid = true;
            return itr;
        }
    }
    if (auto d = new ProcessData)
    {
        d->name;
        d->sock = -1;
        d->bufflen = 0;
        d->detected = false;
        d->lastTm = ProcessManager::msTimeTick();
        d->valid = true;
        m_processesData.push_back(d);
        return d;
    }
    return NULL;
}
