#include "gsocketmanager.h"
#include "gsocket.h"
#include "ObjectManagers.h"
#include "Utility.h"
#include "GLibrary.h"
#if defined _WIN32 || defined _WIN64
#include <conio.h>
#endif //defined _WIN32 || defined _WIN64

int main()
{
#if defined _WIN32 || defined _WIN64
    //_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
#endif //defined _WIN32 || defined _WIN64
    GLibrary l2("uavandqgc", GLibrary::CurrentPath());
    ISocketManager *m = GSocketManager::CreateManager(1);
    GSocket *s = new GSocket(NULL);
    if (s)
    {
        s->Bind(1003, "");
        m->AddSocket(s);
    }

    while (m->IsRun())
    {
#if defined _WIN32 || defined _WIN64
        if (_kbhit() > 0)
        {
            int ch = _getch();
            if (ch == 27)
                m->Exit();
        }
#endif //defined _WIN32 || defined _WIN64
        m->Poll(50);
    }
    delete s;
    delete m;
    return 0;
}