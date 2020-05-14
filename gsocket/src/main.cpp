#include "gsocketmanager.h"
#include "gsocket.h"
#include "ObjectManagers.h"
#include "Utility.h"
#include "GLibrary.h"
#if defined _WIN32 || defined _WIN64
#include <conio.h>
#else
#include <signal.h>  
#endif //defined _WIN32 || defined _WIN64
#include "ILog.h"
#include "protectsocket.h"

#define DefaultPort 8198
static ISocketManager *sSockMgr = NULL;
static void dump(int signo)
{
    Utility::Dump("dump.txt", signo);
    sSockMgr->CloseServer();
    exit(-1);
}

void OnBindFinish(ISocket *sock, bool binded)
{
    if(sock)
    {
        char buff[256];
        sprintf(buff, "bind %s:%d %s", sock->GetHost().c_str(), sock->GetPort(), binded ? "success" : "fail");
        GSocket::GetLog().Log(buff, "Listen", 0, errno);
    }
    exit(0);
}

int main(int argc, char *argv[])
{
#if defined _WIN32 || defined _WIN64
    _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
#else
    signal(SIGSEGV, dump);
    signal(SIGABRT, dump);
#endif //defined _WIN32 || defined _WIN64
    GLibrary lib("uavandqgc", GLibrary::CurrentPath());
    sSockMgr = GSocketManager::CreateManager(1);
    ISocket *sock = new GSocket(NULL);
    ISocket *sockProtect = new ProtectSocket(NULL);
    int port = argc > 1 ? (int)Utility::str2int(argv[1]) : DefaultPort;
    if (port <1000)
        port = DefaultPort;

    if (sock && sockProtect)
    {
        sock->Bind(port, "");
        sockProtect->ConnectTo("127.0.0.1", 27059);
        sSockMgr->AddSocket(sock);
        sSockMgr->AddSocket(sockProtect);
        sSockMgr->SetBindedCB(sock, &OnBindFinish);
    }

    while (sSockMgr->IsRun())
    {
#if defined _WIN32 || defined _WIN64
        if (_kbhit() > 0)
        {
            int ch = _getch();
            if (ch == 27)
                sSockMgr->Exit();
        }
#endif //defined _WIN32 || defined _WIN64
        sSockMgr->Poll(50);
        std::string f = Utility::ModuleName() + " " + Utility::l2string(port);
        switch (sockProtect->GetSocketStat())
        {
        case ISocket::Connected:
            sockProtect->Send(f.size(), f.c_str()); break;
        case ISocket::Closed:
            sockProtect->ConnectTo("127.0.0.1", 31057); break;
        default:
            break;
        }
    }
    lib.Unload();
    delete sock;
    delete sSockMgr;
    return 0;
}