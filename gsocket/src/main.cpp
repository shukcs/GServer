#include "gsocketmanager.h"
#include "gsocket.h"
#include "ObjectManagers.h"
#include "Utility.h"
#include "GLibrary.h"

int main()
{
    std::string str = Utility::base64("uavandqgc", 10);
    uint8_t buff[64] = { 0 };
    size_t n = sizeof(buff);
    Utility::base64d(str, (char*)buff, n);
    GLibrary l2("uavandqgc", GLibrary::CurrentPath());
    GSocket s(NULL);
    GSocketManager m(1);
    s.Bind(1003, "");
    m.AddSocket(&s);

    while (1)
        m.Poll(50);
    return 0;
}