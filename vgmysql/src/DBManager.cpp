#include "DBManager.h"
#include "VGMysql.h"
#include "DBExecItem.h"
#include "DBMessages.h"
#include "tinyxml.h"
#include "Utility.h"
#include "ObjectManagers.h"


#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif
////////////////////////////////////////////////////////////////////////////////
//ObjectDB
////////////////////////////////////////////////////////////////////////////////
ObjectDB::ObjectDB(const std::string &id):IObject(id), MysqlDB()
, m_sqlEngine(new VGMySql)
{
}

ObjectDB::~ObjectDB()
{
    delete m_sqlEngine;
}

ObjectDB *ObjectDB::ParseMySql(const TiXmlElement &e)
{
    const char *name = e.Attribute("name");
    if (!name)
        return NULL;

    ObjectDB *ret = new ObjectDB(name);
    string db = ret->Parse(e);
    return ret;
}

int ObjectDB::GetObjectType() const
{
    return IObject::DBMySql;
}

void ObjectDB::InitObject()
{
    if (m_stInit != Uninitial)
        return;

    m_stInit = IObject::Initialed;
    m_sqlEngine->ConnectMySql(GetDBHost(), GetDBPort(), GetDBUser(), GetDBPswd());
    m_sqlEngine->EnterDatabase(GetDBName());
    for (VGTable *tb : m_tables)
    {
        m_sqlEngine->CreateTable(tb);
    }
    for (VGTrigger *itr : m_triggers)
    {
        m_sqlEngine->CreateTrigger(itr);
    }
}

void ObjectDB::ProcessMessage(const IMessage *msg)
{
    if (m_stInit == Uninitial)
        InitObject();

    auto db = dynamic_cast<const DBMessage *>(msg);
    if (!db)
        return;

    DBMessage *ack = db->GenerateAck(this);
    uint64_t ref = 0;
    int idx = 0;
    for (const string sql : db->GetSqls())
    {
        if (ExecutItem *item = GetSqlByName(sql))
        {
            item->ClearData();
            _initSqlByMsg(*item, *db, idx);
            if (ref > 0)
            {
                _initRefField(*item, db->GetRefFiled(), ref);
                ref = 0;
            }

            ref = _executeSql(item, ack, idx);
        }
        idx++;
    }
    if (ack)
        SendMsg(ack);
}

void ObjectDB::_initSqlByMsg(ExecutItem &sql, const DBMessage &msg, int idx)
{
    for (FiledVal *fd : sql.Fields(ExecutItem::Write))
    {
        string name = fd->GetFieldName();
        if (msg.HasWrite(name, idx))
            _initFieldByVarient(*fd, msg.GetWrite(name, idx));
    }
    for (FiledVal *fd : sql.Fields(ExecutItem::Condition))
    {
        string name = fd->GetFieldName();
        if (auto tmp = fd->ComplexSql())
            _initCmpSqlByMsg(*tmp, msg, idx);
        else if (msg.HasCondition(name, idx))
            _initFieldByVarient(*fd, msg.GetCondition(name, idx, fd->GetJudge()));
    }
    if (sql.GetType() != ExecutItem::Select)
        return;

    sql.SetLimit(msg.GetLimitBeg(), msg.GetLimit());
}

void ObjectDB::_initCmpSqlByMsg(ExecutItem &sql, const DBMessage &msg, int idx)
{
    for (FiledVal *fd : sql.Fields(ExecutItem::Write))
    {
        string name = sql.GetName() + "." + fd->GetFieldName();
        _initFieldByVarient(*fd, msg.GetWrite(name, idx));
    }
    for (FiledVal *fd : sql.Fields(ExecutItem::Condition))
    {
        string name = sql.GetName()+ "." + fd->GetFieldName();
        _initFieldByVarient(*fd,  msg.GetCondition(name, idx));
    }
}

void ObjectDB::_initRefField(ExecutItem &sql, const std::string &field, uint64_t idx)
{
    if (field.empty())
        return;

    if (FiledVal *fd = sql.GetWriteItem(field))
        fd->InitOf(idx);
}

int64_t ObjectDB::_executeSql(ExecutItem *sql, DBMessage *msg, int idx)
{
    uint64_t ret = 0;
    bool bRes = m_sqlEngine->Execut(sql);
    if (bRes && msg)
    {
        FiledVal *fd = sql->GetIncrement();
        if (fd && msg)
        {
            ret = fd->GetValue();
            msg->SetRead(INCREASEField, ret, idx);
        }

        if ( sql->GetType()==ExecutItem::Select)
        {
            do {
                for (FiledVal *itr : sql->Fields(ExecutItem::Read))
                {
                    if (msg)
                        _save2Message(*itr, *msg, idx);
                }
            } while (m_sqlEngine->GetResult());
        }
    }
    if (msg)
        msg->SetRead(EXECRSLT, bRes, idx);

    return ret;
}

void ObjectDB::_initFieldByVarient(FiledVal &fd, const Variant &v)
{
    switch (v.GetType())
    {
    case Variant::Type_string:
        fd.SetParam(v.ToString()); break;
    case Variant::Type_int32:
    case Variant::Type_uint32:
        fd.InitOf(v.ToInt32()); break;
    case Variant::Type_int16:
    case Variant::Type_uint16:
        fd.InitOf(v.ToInt16()); break;
    case Variant::Type_int8:
    case Variant::Type_uint8:
        fd.InitOf(v.ToInt8()); break;
    case Variant::Type_int64:
    case Variant::Type_uint64:
        fd.InitOf(v.ToInt64()); break;
    case Variant::Type_double:
        fd.InitOf(v.ToDouble()); break;
    case Variant::Type_float:
        fd.InitOf(v.ToFloat()); break;
    case Variant::Type_buff:
        fd.InitBuff(v.GetBuffLength(), v.GetBuff()); break;
    case Variant::Type_StringList:
        fd.SetParam(v.ToStringList()); break;
    case Variant::Type_bool:
        fd.SetNotStringParam(v.ToBool() ? "1" : "0"); break;
    case Variant::Type_Unknow:
        fd.SetNotStringParam("NULL"); break;
    default:
        break;
    }
}

void ObjectDB::_save2Message(const FiledVal &fd, DBMessage &msg, int idx)
{
    if (fd.IsEmpty() && !msg.IsQueryList())
        return;

    Variant v;
    switch (fd.GetParamType() & 0xff)
    {
    case FiledVal::Int32:
        v = fd.IsEmpty() ? Variant(Variant::Type_int32) : fd.GetValue<int32_t>(); break;
    case FiledVal::Int64:
        v = fd.IsEmpty() ? Variant(Variant::Type_int64) : fd.GetValue<int64_t>(); break;
    case FiledVal::Int16:
        v = fd.IsEmpty() ? Variant(Variant::Type_int16) : fd.GetValue<int16_t>(); break;
    case FiledVal::Int8:
        v = fd.IsEmpty() ? Variant(Variant::Type_int8) : fd.GetValue<int8_t>(); break;
    case FiledVal::Double:
        v = fd.IsEmpty() ? Variant(Variant::Type_double) : fd.GetValue<double>(); break;
    case FiledVal::Float:
        v = fd.IsEmpty() ? Variant(Variant::Type_float) : fd.GetValue<float>(); break;
    case FiledVal::String:
        v = fd.IsEmpty() ? Variant(Variant::Type_string) : string((char*)fd.GetBuff(), fd.GetValidLen()); break;
    case FiledVal::Buff:
        v = fd.IsEmpty() ? Variant(Variant::Type_buff) : Variant(fd.GetValidLen(), (char*)fd.GetBuff()); break;
        break;
    }

    msg.IsQueryList() ? msg.AddRead(fd.GetFieldName(), v, idx): msg.SetRead(fd.GetFieldName(), v, idx);
}
////////////////////////////////////////////////////////////////////////////////
//DBManager
////////////////////////////////////////////////////////////////////////////////
DBManager::DBManager():IObjectManager()
{
}

DBManager::~DBManager()
{
}

int DBManager::GetObjectType() const
{
    return IObject::DBMySql;
}

bool DBManager::PrcsPublicMsg(const IMessage &)
{
    return true;
}

IObject *DBManager::PrcsNotObjectReceive(ISocket *, const char *, int)
{
    return NULL;
}

void DBManager::LoadConfig(const TiXmlElement *root)
{
    const TiXmlElement *cfg = root ? root->FirstChildElement("DBManager") : NULL;
    if (!cfg)
        return;

    InitThread(1, 0);
    const TiXmlElement *dbNode = cfg->FirstChildElement("Object");
    while (dbNode)
    {
        if (ObjectDB *db = ObjectDB::ParseMySql(*dbNode))
            AddObject(db);

        dbNode = dbNode->NextSiblingElement("Object");
    }
}

bool DBManager::IsReceiveData() const
{
    return false;
}


DECLARE_MANAGER_ITEM(DBManager)