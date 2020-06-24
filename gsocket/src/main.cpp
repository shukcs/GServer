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
#include "tinyxml.h"

static ISocketManager *sSockMgr = NULL;
static void dump(int signo)
{
    auto fileName = Utility::dateString(Utility::msTimeTick()) + ".txt";
    Utility::Dump("dump"+fileName, signo);
    sSockMgr->CloseServer();
    exit(-1);
}

void OnBindFinish(ISocket *sock, bool binded)
{
    if(sock)
    {
        char buff[256];
        snprintf(buff, 256, "bind %s:%d %s", sock->GetHost().c_str(), sock->GetPort(), binded ? "success" : "fail");
        GSocket::GetLog().Log(buff, "Listen", 0, errno);
    }
    if (!binded)
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
    TiXmlDocument doc;
    doc.LoadFile("config.xml");
    const TiXmlElement *rootElement = doc.RootElement();
    if (!rootElement)
        return -1;

    std::list<GLibrary*> lsLib;
    if (const TiXmlElement *libs = rootElement->FirstChildElement("Libs"))
    {
        const TiXmlNode *lib = libs->FirstChild("Library");
        while (lib)
        {
            if (const char *name = lib->ToElement()->Attribute("name"))
                lsLib.push_back(new GLibrary(name, GLibrary::CurrentPath()));
            lib = lib->NextSibling("Library");
        }
    }

    if (const TiXmlElement *socketCfg = rootElement->FirstChildElement("GSocketManager"))
    {
        const char *tmp = socketCfg->Attribute("thread");
        int nThread = tmp ? int(Utility::str2int(tmp)) : 1;
        tmp = socketCfg->Attribute("maxListen");
        int maxListen = tmp ? int(Utility::str2int(tmp)) : 100000;
        sSockMgr = GSocketManager::CreateManager(nThread, maxListen);
        tmp = socketCfg->Attribute("portListen");
        if (tmp)
        {
            for (const std::string &itr : Utility::SplitString(tmp, ";"))
            {
                if (ISocket *sock = new GSocket(sSockMgr))
                {
                    sock->Bind(int(Utility::str2int(itr)), "");
                    sSockMgr->AddSocket(sock);
                    sSockMgr->SetBindedCB(sock, &OnBindFinish);
                }
            }
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
        delete sSockMgr;
        return 0;
    }

    for (auto itr : lsLib)
    {
        if (itr)
            itr->Unload();
        delete itr;
    }
    return -1;
}