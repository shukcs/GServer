#ifndef __VG_TABLE_H__
#define __VG_TABLE_H__

#include "sqlconfig.h"
#include <string>
#include <list>
#if !defined _WIN32 && !defined _WIN64
#include <strings.h>
#define strnicmp strncasecmp
#endif

using namespace std;

typedef list<string> StringList;
class VGTable;
class VGForeignKey;
class TiXmlElement;
class MysqlDB;

class VGTableField
{
public:
    string GetName(bool bTable=false)const;
    void SetName(const string &n);
    void SetConstrains(const string &n);
    const StringList &GetConstrains()const;
    const string &GetTypeName()const;

    int GetType();
    int GetLength()const;
    bool IsValid()const;
    string ToString()const;
public:
    static VGTableField *ParseFiled(const TiXmlElement &e, VGTable *parent);
    static int GetTypeByName(const string &type);
    static int TransSqlType(int t);
    static int TransBuffLength(int t);
protected:
    VGTableField(VGTable *tb, const string &n = string());
private:
    static int _parseBits(const string &n);
    static int _parseChar(const string &n);
    static int _parseVarChar(const string &n);
    static int _parseBinary(const string &n);
    static int _parseVarBinary(const string &n);
private:
    VGTable     *m_table;
    int         m_type;
    string      m_name;
    StringList  m_constraints;
    string      m_typeName;
};

class VGTable
{
public:
    const string &GetName()const;
    const list<VGTableField *> &Fields()const;
    VGTableField *FindFieldByName(const string &n);
    string ToCreateSQL()const;
    void AddField(VGTableField *);
    bool IsForeignRef()const;
public:
    static VGTable *ParseTable(const TiXmlElement &e, const MysqlDB &db);
protected:
    VGTable(const string &n = string());
    void AddForeign(VGForeignKey *f, const MysqlDB &db);
private:
    string                  m_name;
    bool                    m_bForeignRef;
    list<VGTableField *>    m_fields;
    list<VGForeignKey *>    m_foreigns;
};

#endif // __VG_TABLE_H__