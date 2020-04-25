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

#define DefaultPort 8198
static ISocketManager *sSockMgr = NULL;

static void dump(int signo)
{
    Utility::Dump("dump.txt", signo);
}

void OnBindFinish(ISocket *sock, bool binded)
{
    if(sock)
    {
        char buff[1024];
        sprintf(buff, "bind %s:%d %s", sock->GetHost().c_str(), sock->GetPort(), binded ? "success" : "fail");
        GSocket::GetLog().Log(buff, "Listen", 0, errno);
    }

    if (!binded && sSockMgr)
        sSockMgr->Exit();
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
    GSocket *s = new GSocket(NULL);
    int port = argc > 1 ? (int)Utility::str2int(argv[1]) : DefaultPort;
    if (port <1000)
        port = DefaultPort;
    if (s)
    {
        s->Bind(port, "");
        sSockMgr->AddSocket(s);
        sSockMgr->SetBindedCB(s, &OnBindFinish);
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
    }
    lib.Unload();
    delete s;
    delete sSockMgr;
    return 0;
}