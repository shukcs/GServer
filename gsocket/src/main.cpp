#include "gsocketmanager.h"
#include "gsocket.h"
#include "ObjectManagers.h"
#include "Utility.h"
#include "GLibrary.h"
#if defined _WIN32 || defined _WIN64
#include <conio.h>
#endif //defined _WIN32 || defined _WIN64

static ISocketManager *sSockMgr = NULL;
void OnBindFinish(ISocket *sock, bool binded)
{
    if(sock && sSockMgr)
        sSockMgr->Log(binded ? 0 : errno, "Listen", 0, "bind %s:%d %s",
            sock->GetHost().c_str(), sock->GetPort(), binded ? "success" : "fail");

    if (!binded && sSockMgr)
        sSockMgr->Exit();
}

int main()
{
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