#include "gsocketmanager.h"
#include "gsocket.h"
#include "ObjectManagers.h"
#include "Utility.h"
#include "GLibrary.h"
//#include "VGDBManager.h"
//#include "VGMysql.h"
//#include "DBExecItem.h"

int main()
{
    //GLibrary l("vgmysql", GLibrary::CurrentPath());
    GSocket s(NULL);
    GSocketManager m(1);
    s.Bind(1003, "");
    m.AddSocket(&s);
    //VGMySql mysql;
    //mysql.ConnectMySql("127.0.0.1", 3306, "root", "", "gsuav");
    /*if (ExecutItem *item = VGDBManager::Instance().GetSqlByName("insertLand"))
    {
        item->ClearData();
        if (FiledValueItem *fd = item->GetWriteItem(ExecutItem::Write, "gsuser"))
            fd->SetParam("huangsj");

        mysql.Execut(item);
    }*/
    GLibrary l2("uavandqgc", GLibrary::CurrentPath());
    while (1)
        m.Poll(50);
    return 0;
}