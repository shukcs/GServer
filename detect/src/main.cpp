#if defined _WIN32 || defined _WIN64
#include <WinSock2.h>
#include <windows.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/ioctl.h>
#endif
#include "processmanager.h"

#if !defined _WIN32 && !defined _WIN64
#define MAX_PATH 1024
#define closesocket close
#else
typedef int socklen_t;
#endif


bool SetNonblocking(int h)
{
    if (-1 == h)
        return false;
#if defined _WIN32 || defined _WIN64
    unsigned long ul = 1;
    if (SOCKET_ERROR == ioctlsocket(h, FIONBIO, &ul))
#else
    if (0 != ioctl(h, FIONBIO, 1))//设置成非阻塞模式；
#endif
        return false;
    return true;
}

void Sleep(int ms)
{
#if defined _WIN32 || defined _WIN64
    ::Sleep(ms);
#else
    struct timeval tv;
    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;
    select(0, NULL, NULL, NULL, &tv);
#endif
}

int main(int argc, char *argv[])
{
    ProcessManager mgr;
    mgr.AddProcess("gsocket 18198");
    const int listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (-1 == listenfd)
        return -1;
    struct sockaddr_in saddr = {0};
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(27059);
    saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    socklen_t len = sizeof(saddr);
    if (-1 == bind(listenfd, (sockaddr *)&saddr, len))
    {
        closesocket(listenfd);
        return 0;
    }

    mgr.AddSocket(listenfd);
    listen(listenfd, 20);
    fd_set rfds = { 0 };
    if (!FD_ISSET(listenfd, &rfds))
        FD_SET(listenfd, &rfds);
    SetNonblocking(listenfd);
    char buff[1024];
    while (1)
    {
        struct timeval tv = {0};
        tv.tv_usec = 1000000;
        fd_set fdTmp = rfds;
        if (select(mgr.MaxFD(), &fdTmp, NULL, NULL, &tv) > 0)
        {
            for (int curFd : mgr.Sockets())
            {
                if (!FD_ISSET(curFd, &fdTmp))
                    continue;

                if (listenfd == curFd)
                {
                    int connfd = accept(listenfd, (sockaddr *)&saddr, &len); //accept这个连接
                    if (-1 == connfd)
                        return 0;

                    FD_SET(connfd, &rfds);
                    mgr.AddSocket(connfd);
                    SetNonblocking(connfd);
                }
                else
                {
#if defined _WIN32 ||  defined _WIN64
                    int n = recv(curFd, buff, sizeof(buff), 0);
#else
                    int n = read(curFd, buff, sizeof(buff));
#endif
                    if (n < 0)//关闭触发EPOLLIN，但接收不到数据
                    {
                        closesocket(curFd);
                        FD_CLR(curFd, &rfds);
                        mgr.RemoveSocket(curFd);
                        continue;
                    }
                    mgr.Receive(curFd, buff, n);
                }
            }
        }
        mgr.CheckProcess(mgr.msTimeTick());
    }
    return 0;
}