#ifndef __DB_EXEC_ITEM__
#define __DB_EXEC_ITEM__

#include "VGTable.h"
typedef struct st_mysql_bind MYSQL_BIND;
class TiXmlElement;
class ExecutItem;
class MysqlDB;

enum BraceFlag {
    NoBrace,
    NumbLeftMask = 0xff,//×î×ó+'('
    NumbRightMask = 0xff00,//×îÓÒ+')'
};

class FiledVal
{
public:
    enum FieldType{
        NoBuff,
        Int32,
        Int64,
        Int16,
        Int8,
        Buff,
        Double,
        Float,
        String,
        StaticParam = 0x100,
        StaticRef = StaticParam << 1,
        List = StaticRef<<1,
    };
public:
    void SetNotStringParam(const string &param);
    void SetParam(const string &param, FieldType tp = NoBuff);
    void SetParam(const string &exFild, const string &param, FieldType tp = NoBuff);
    void SetParam(const list<string> &param);
    void SetParam(const string &exFild, const list<string> &param);
    unsigned GetMaxLen()const;
    unsigned GetValidLen()const;
    void *GetBuff()const;
    FieldType GetParamType()const;
    void InitBuff(unsigned len, const void *buf = NULL);
    const string &GetFieldName()const;
    const string &GetJudge()const;
    ExecutItem *ComplexSql()const;
    bool IsEmpty()const;

    string ToConditionString(const string &str)const;
    int GetType()const;
    void SetFieldName(const string &name);
    bool IsStringParam()const;
    void SetEmpty();
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
        if (m_len < sizeof(T) || m_bEmpty)
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
    static void parse(const TiXmlElement *e, ExecutItem *tb, const MysqlDB &db);
    static int transToType(const char *pro);
protected:
    static FiledVal *parseFiled(const MysqlDB &db, const TiXmlElement &e, bool oth=false);
    static BraceFlag getBraceFlag(const string &str);
    static string finallyString(const string &str, BraceFlag f);
protected:
    FiledVal(int tp, const string &name = "", int len = 0);
    FiledVal(VGTableField *fild, bool bOth = false);
    virtual ~FiledVal();
    void transType(int sqlType);
private:
    friend class ExecutItem;
    int             m_type;
    char            *m_buff;
    ExecutItem      *m_exParam;
    unsigned long   m_lenMax;
    unsigned long   m_len;
    FieldType       m_tpField;
    char            m_bEmpty;
    string          m_name;
    string          m_condition;
    string          m_param;
};

class ExecutItem
{
public:
    enum ExecutType {
        Unknown=0,
        Insert,
        Replace,
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

    FiledVal *GetReadItem(const string &name)const;
    FiledVal *GetWriteItem(const string &name)const;
    FiledVal *GetConditionItem(const string &name)const;
    FiledVal *GetIncrement()const;
    const list<FiledVal*> &Fields(FiledType tp)const;
    bool IsValid()const;
    int CountRead()const;
    void ClearData();
    ExecutType GetType()const;
    const std::string &GetName()const;
    void SetLimit(uint32_t beg=0, uint16_t limit=200);

    string GetSqlString(MYSQL_BIND *paramBinds=NULL)const;
    MYSQL_BIND *GetParamBinds();
    const StringList &ExecutTables()const;
    void AddItem(FiledVal *item, int tp);
    void SetIncrement(FiledVal *item);
    FiledVal *GetItem(const string &name, FiledType tp)const;
    MYSQL_BIND *TransformRead();
    bool HasForeignRefTable()const;
    void SetRef(bool b);
    bool IsRef()const;
public:
    static void transformBind(FiledVal *item, MYSQL_BIND &bind, bool=false);
    static ExecutItem *parse(const TiXmlElement *e, const MysqlDB &db);
    static ExecutType transToSqlType(const char *pro);
protected:
    ExecutItem(ExecutType tp, const std::string &name);
    void SetExecutTables(const StringList &tbs, const MysqlDB &db);
    void AddExecutTable(const std::string &t, const MysqlDB &db);
private:
    void _parseItems(const TiXmlElement *e, const MysqlDB &db);
    void _addCondition(FiledVal *item);
    string _getTablesString()const;
    string _toInsert(MYSQL_BIND *bind, int &pos)const;
    string _toReplace(MYSQL_BIND *bind, int &pos)const;
    string _toDelete(MYSQL_BIND *bind, int &pos)const;
    string _toUpdate(MYSQL_BIND *bind, int &pos)const;
    string _toSelect(MYSQL_BIND *param,int &pos)const;

    string _conditionsString(MYSQL_BIND *bind, int &pos)const;
    string _deleteBegin()const;
    string _genWrite(MYSQL_BIND *bind, int &pos)const;
    string _getCondition(const std::string &str)const;
private:
    ExecutType        m_type;
    string            m_name;
    FiledVal          *m_autoIncrement;
    char              m_bHasForeignRefTable;
    char              m_bRef;
    uint16_t          m_nlimit;
    uint32_t          m_limitBeg;
    string            m_group;
    StringList        m_tables;
    StringList        m_conditions;
    list<FiledVal*>   m_itemsRead;
    list<FiledVal*>   m_itemsWrite;
    list<FiledVal*>   m_itemsCondition;
};

#endif //__DB_EXEC_ITEM__;
