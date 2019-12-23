#include "gsocketmanager.h"
#include "gsocket.h"
#include "ObjectManagers.h"
#include "Utility.h"
#include "GLibrary.h"
#if defined _WIN32 || defined _WIN64
#include <conio.h>
#endif //defined _WIN32 || defined _WIN64
#include "LoopQueue.h"

static ISocketManager *sSockMgr = NULL;
void OnBindFinish(ISocket *sock, bool binded)
{
    if(sock)
        GSocket::Log(binded ? 0 : errno, "Listen", 0, "bind %s:%d %s",
            sock->GetHost().c_str(), sock->GetPort(), binded ? "success" : "fail");

    if (!binded && sSockMgr)
        sSockMgr->Exit();
}

int main()
{
    LoopQueBuff que;
    que.ReSize(2048);
    char buff[1024] = {0};
    for (int i = 0; i<1000; ++i)
    {
        for (int j = 0; j <= 35; ++j)
            que.Push("1234567890\r\n", 12);
        int tmp = que.CopyData(buff, 1024);
        if (tmp < 1024)
            que.Clear();
    }
#if defined _WIN32 || defined _WIN64
    //_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
#endif //defined _WIN32 || defined _WIN64
    GLibrary l2("uavandqgc", GLibrary::CurrentPath());
    sSockMgr = GSocketManager::CreateManager(1);
    GSocket *s = new GSocket(NULL);
    if (s)
    {
        s->Bind(8198, "");
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
    delete s;
    delete sSockMgr;
    return 0;
}