#ifndef __VG_DB_MANAGER_H__
#define __VG_DB_MANAGER_H__

#include "sqlconfig.h"
#include <list>
#include <string>

class ExecutItem;
class VGTable;
class TiXmlElement;

class VGDBManager {
public:
    SHARED_SQL bool Load(const std::string &fileName);
    SHARED_SQL VGTable *GetTableByName(const std::string &name)const;
    SHARED_SQL ExecutItem *GetSqlByName(const std::string &name)const;
    SHARED_SQL const std::list<VGTable*> &Tables()const;
public:
    static SHARED_SQL VGDBManager &Instance();
    static SHARED_SQL long str2int(const std::string &str, unsigned radix=10, bool *suc=NULL);
    static SHARED_SQL std::list<std::string> SplitString(const std::string &str, const std::string &sp, bool bSkipEmpty = true);
private:
    VGDBManager();

    void parseTable(TiXmlElement *e);
    void parseSql(TiXmlElement *e);
private:
    std::list<VGTable*>     m_tables;
    std::list<ExecutItem*>  m_sqls;
};

#endif // __VG_DB_MANAGER_H__