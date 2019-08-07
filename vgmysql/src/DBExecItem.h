#ifndef __DB_EXEC_ITEM__
#define __DB_EXEC_ITEM__

#include "VGTable.h"
typedef struct st_mysql_bind MYSQL_BIND;
class TiXmlElement;
class ExecutItem;
class FiledValueItem {
public:
    SHARED_SQL FiledValueItem(int tp, const string &name = "", int len = 0);
    SHARED_SQL FiledValueItem(VGTableField *fild, bool bOth=false);
    SHARED_SQL ~FiledValueItem();

    SHARED_SQL void SetFieldName(const string &name);
    SHARED_SQL const string &GetFieldName()const;
    SHARED_SQL void SetParam(const string &param, bool=true, bool =false);
    SHARED_SQL const string &GetParam()const;
    SHARED_SQL void InitBuff(unsigned len, const void *buf = NULL);
    SHARED_SQL unsigned GetMaxLen()const;
    SHARED_SQL unsigned GetValidLen()const;
    SHARED_SQL void *GetBuff()const;
    SHARED_SQL int GetType()const;
    SHARED_SQL unsigned long &ReadLength();
    SHARED_SQL bool IsStaticParam()const;
    SHARED_SQL bool IsEmpty()const;
    SHARED_SQL void SetEmpty();
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
    static void parse(TiXmlElement *e, ExecutItem *tb);
private:
    static int _transToType(const char *pro);
private:
    int             m_type;
    bool            m_bEmpty;
    bool            m_bStatic;
    char            *m_buff;
    unsigned long   m_lenMax;
    unsigned long   m_len;
    string          m_name;
    string          m_param;
};

class ExecutItem {
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
    SHARED_SQL ExecutItem(ExecutType tp, const std::string &name);
    SHARED_SQL ~ExecutItem();

    SHARED_SQL ExecutType GetType()const;
    SHARED_SQL const std::string &GetName()const;
    SHARED_SQL const StringList &ExecutTables()const;
    SHARED_SQL void AddExecutTable(const std::string &t);

    SHARED_SQL void AddItem(FiledValueItem *item, int tp);
    SHARED_SQL void SetIncrement(FiledValueItem *item);
    SHARED_SQL FiledValueItem *GetReadItem(const string &name)const;
    SHARED_SQL FiledValueItem *GetWriteItem(const string &name)const;
    SHARED_SQL FiledValueItem *GetConditionItem(const string &name)const;
    SHARED_SQL FiledValueItem *GetIncrement()const;
    SHARED_SQL bool IsValid()const;
    SHARED_SQL FiledValueItem *GetItem(const string &name, int tp)const;
    SHARED_SQL int CountParam()const;
    SHARED_SQL int CountRead()const;
    SHARED_SQL string GetSqlString(MYSQL_BIND *bind, int &pos)const;
    SHARED_SQL void ClearData();
    MYSQL_BIND *TransformRead();
public:
    static void transformBind(FiledValueItem *item, MYSQL_BIND &bind, bool=false);
    static ExecutItem *parse(TiXmlElement *e);
private:
    void _parseItems(TiXmlElement *e);
    void _addCondition(FiledValueItem *item);
    string _getTablesString()const;
    string _toInsert(MYSQL_BIND *bind, int &pos)const;
    string _toDelete(MYSQL_BIND *bind, int &pos)const;
    string _toUpdate(MYSQL_BIND *bind, int &pos)const;
    string _toSelect(MYSQL_BIND *param,int &pos)const;

    string _conditionsString(MYSQL_BIND *bind, int &pos)const;
    string _deleteBegin()const;
private:
    static ExecutType _transToType(const char *pro);
private:
    ExecutType              m_type;
    string                  m_name;
    FiledValueItem*         m_autoIncrement;
    StringList              m_tables;
    list<FiledValueItem*>   m_itemsRead;
    list<FiledValueItem*>   m_itemsWrite;
    list<FiledValueItem*>   m_itemsCondition;
};

#endif //__DB_EXEC_ITEM__;
