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

class VGTableField {
public:
    string GetName(bool bTable=false)const;
    void SetName(const string &n);
    void SetConstrains(const string &n);
    const StringList &GetConstrains()const;
    const string &GetTypeName()const;
    void SetTypeName(const string &type);

    int GetType();
    int GetLength()const;
    bool IsValid()const;
    string ToString()const;
public:
    static VGTableField *ParseFiled(TiXmlElement &e, VGTable *parent);
protected:
    VGTableField(VGTable *tb, const string &n = string());
private:
    void _parseChar(const string &n);
    void _parseVarChar(const string &n);
    void _parseBinary(const string &n);
    void _parseVarBinary(const string &n);
private:
    VGTable     *m_table;
    int         m_type;
    string      m_name;
    StringList  m_constraints;
    string      m_typeName;
};

class VGTable {
public:
    const string &GetName()const;
    const list<VGTableField *> &Fields()const;
    VGTableField *FindFieldByName(const string &n);
    string ToCreateSQL()const;
    void AddField(VGTableField *);
    bool IsForeignRef()const;
public:
    static VGTable *ParseTable(TiXmlElement &e);
protected:
    VGTable(const string &n = string());
    void AddForeign(VGForeignKey *f);
private:
    string                  m_name;
    bool                    m_bForeignRef;
    list<VGTableField *>    m_fields;
    list<VGForeignKey *>    m_foreigns;
};

#endif // __VG_TABLE_H__