#ifndef __PROCESSMANAGER_H__
#define __PROCESSMANAGER_H__
#include <string>
#include <list>

typedef struct _processData ProcessData;
typedef std::list<std::string> StringList;
class ProcessManager
{
public:
    ProcessManager();
    void AddProcess(const std::string &p);
    void AddSocket(int fd);
    void RemoveSocket(int fd);
    void Receive(int sock, const char *buff, int len);
    void CheckProcess(int64_t ms);
    const std::list<int> &Sockets()const;
    int MaxFD()const;
public:
    static int64_t msTimeTick();
protected:
    ProcessData *GetProcess(const std::string &p)const;
    ProcessData *GetProcess(int sock)const;
    ProcessData *GetInitProcess();
protected:
    std::list<ProcessData*> m_processesData;
    StringList              m_processes;
    std::list<int>          m_sockets;
};

#endif //__PROCESSMANAGER_H__