#include "gsocketmanager.h"
#include "gsocket.h"
#include "ObjectManagers.h"
#include "Utility.h"
#include "GLibrary.h"

int main()
{
    GLibrary l2("uavandqgc", GLibrary::CurrentPath());
    GSocket s(NULL);
    ISocketManager *m = GSocketManager::CreateManager(1);
    s.Bind(1003, "");
    m->AddSocket(&s);

    while (m->IsRun())
    {
        m->Poll(50);
    }

    return 0;
}