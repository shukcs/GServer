#ifndef __DB_EXEC_ITEM__
#define __DB_EXEC_ITEM__

#include "VGTable.h"
typedef struct st_mysql_bind MYSQL_BIND;
class TiXmlElement;
class ExecutItem;
class FiledValueItem
{
public:
    SHARED_SQL void SetParam(const string &param, bool=true, bool =false);
    SHARED_SQL unsigned GetMaxLen()const;
    SHARED_SQL unsigned GetValidLen()const;
    SHARED_SQL void *GetBuff()const;
    SHARED_SQL void InitBuff(unsigned len, const void *buf = NULL);

    int GetType()const;
    void SetFieldName(const string &name);
    bool IsStaticParam()const;
    bool IsEmpty()const;
    void SetEmpty();
    const string &GetFieldName()const;
    const string &GetParam()const;
    unsigned long &ReadLength();
public:
    template<class T>
    void InitOf(const T &t)
    {
        InitBuff(sizeof(T), &t);
    }

    template<typename T=uint64_t>
    T GetValue(bool *suc=NULL)const
    {
        T ret(0);
        if (m_len < sizeof(T))
        {
            if (suc)
                *suc = false;
            return ret;
        }

        if (suc)
            *suc = true;

        ret = *(T*)m_buff;
        return ret;
    }
public:
    static void parse(const TiXmlElement *e, ExecutItem *tb);
    static int transToType(const char *pro);
protected:
    FiledValueItem(int tp, const string &name = "", int len = 0);
    FiledValueItem(VGTableField *fild, bool bOth = false);
    ~FiledValueItem();
private:
    friend class ExecutItem;
    int             m_type;
    bool            m_bEmpty;
    bool            m_bStatic;
    char            *m_buff;
    unsigned long   m_lenMax;
    unsigned long   m_len;
    string          m_name;
    string          m_param;
};

class ExecutItem
{
public:
    enum ExecutType {
        Unknown=0,
        Insert,
        Delete,
        Update,
        Select,
    };
    enum FiledType {
        Read = 0,
        Write,
        Condition,
        AutoIncrement,
    };
public:
    ~ExecutItem();

    SHARED_SQL FiledValueItem *GetReadItem(const string &name)const;
    SHARED_SQL FiledValueItem *GetWriteItem(const string &name)const;
    SHARED_SQL FiledValueItem *GetConditionItem(const string &name)const;
    SHARED_SQL FiledValueItem *GetIncrement()const;
    SHARED_SQL bool IsValid()const;
    SHARED_SQL int CountRead()const;
    SHARED_SQL void ClearData();
    SHARED_SQL ExecutType GetType()const;

    string GetSqlString(MYSQL_BIND *paramBinds=NULL)const;
    MYSQL_BIND *GetParamBinds();
    const std::string &GetName()const;
    const StringList &ExecutTables()const;
    void AddItem(FiledValueItem *item, int tp);
    void SetIncrement(FiledValueItem *item);
    FiledValueItem *GetItem(const string &name, int tp)const;
    MYSQL_BIND *TransformRead();
    bool HasForeignRefTable()const;
    void SetRef(bool b);
    bool IsRef()const;
public:
    static void transformBind(FiledValueItem *item, MYSQL_BIND &bind, bool=false);
    static ExecutItem *parse(const TiXmlElement *e);
    static ExecutType transToSqlType(const char *pro);
protected:
    ExecutItem(ExecutType tp, const std::string &name);
    void SetExecutTables(const StringList &tbs);
    void AddExecutTable(const std::string &t);
private:
    void _parseItems(const TiXmlElement *e);
    void _addCondition(FiledValueItem *item);
    string _getTablesString()const;
    string _toInsert(MYSQL_BIND *bind, int &pos)const;
    string _toDelete(MYSQL_BIND *bind, int &pos)const;
    string _toUpdate(MYSQL_BIND *bind, int &pos)const;
    string _toSelect(MYSQL_BIND *param,int &pos)const;

    string _conditionsString(MYSQL_BIND *bind, int &pos)const;
    string _deleteBegin()const;
private:
    ExecutType              m_type;
    string                  m_name;
    FiledValueItem*         m_autoIncrement;
    bool                    m_bHasForeignRefTable;
    bool                    m_bRef;
    StringList              m_tables;
    list<FiledValueItem*>   m_itemsRead;
    list<FiledValueItem*>   m_itemsWrite;
    list<FiledValueItem*>   m_itemsCondition;
};

#endif //__DB_EXEC_ITEM__;
