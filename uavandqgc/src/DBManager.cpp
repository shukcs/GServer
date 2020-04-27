#include "DBManager.h"
#include "ObjectManagers.h"
#include "VGMysql.h"
#include "DBExecItem.h"
#include "DBMessages.h"
#include "tinyxml.h"
#include "Utility.h"


#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif
////////////////////////////////////////////////////////////////////////////////
//ObjectDB
////////////////////////////////////////////////////////////////////////////////
ObjectDB::ObjectDB(const std::string &id):IObject(NULL, id), MysqlDB()
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

void ObjectDB::OnConnected(bool)
{
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

void ObjectDB::ProcessMessage(IMessage *msg)
{
    DBMessage *db = dynamic_cast<DBMessage *>(msg);
    if (!db)
        return;

    DBMessage *ack = db->GenerateAck(this);
    int idx = db->GetSqls().size()>1 ? 1 : 0;
    uint64_t ref = 0;
    for (const string sql : db->GetSqls())
    {
        ExecutItem *item = GetSqlByName(sql);
        int tmp = idx++;
        if (!item)
            continue;
        item->ClearData();
        _initSqlByMsg(*item, *db, tmp);
        if (ref>0)
            _initRefField(*item, db->GetRefFiled(tmp), ref);
        ref = _executeSql(item, ack, tmp);
        if (ref < 1)
            ref = db->GetRead(INCREASEField, tmp).ToUint64();
    }
    if (ack)
        SendMsg(ack);
}

int ObjectDB::ProcessReceive(void *, int len)
{
    return len;
}

void ObjectDB::CheckTimer(uint64_t)
{
}

void ObjectDB::_initSqlByMsg(ExecutItem &sql, const DBMessage &msg, int idx)
{
    for (FiledVal *fd : sql.Fields(ExecutItem::Write))
    {
        string name = fd->GetFieldName();
        _initFieldByVarient(*fd, msg.GetWrite(name, idx));
    }
    for (FiledVal *fd : sql.Fields(ExecutItem::Condition))
    {
        string name = fd->GetFieldName();
        _initFieldByVarient(*fd, msg.GetCondition(name, idx, fd->GetJudge()));
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
                        _save2Message(*itr, *msg);
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
        fd.SetParam(v.ToBool() ? "1":"0"); break;
    default:
        break;
    }
}

void ObjectDB::_save2Message(const FiledVal &fd, DBMessage &msg)
{
    switch (fd.GetParamType() & 0xff)
    {
    case FiledVal::Int32:
        msg.IsQueryList() ? msg.AddRead(fd.GetFieldName(), fd.GetValue<int32_t>())
            : msg.SetRead(fd.GetFieldName(), fd.GetValue<int32_t>());
        break;
    case FiledVal::Int64:
        msg.IsQueryList() ? msg.AddRead(fd.GetFieldName(), fd.GetValue<int64_t>())
            : msg.SetRead(fd.GetFieldName(), fd.GetValue<int64_t>());
        break;
    case FiledVal::Int16:
        msg.IsQueryList() ? msg.AddRead(fd.GetFieldName(), fd.GetValue<int16_t>())
            : msg.SetRead(fd.GetFieldName(), fd.GetValue<int16_t>());
        break;
    case FiledVal::Int8:
        msg.IsQueryList() ? msg.AddRead(fd.GetFieldName(), fd.GetValue<int8_t>())
            : msg.SetRead(fd.GetFieldName(), fd.GetValue<int8_t>());
        break;
    case FiledVal::Double:
        msg.IsQueryList() ? msg.AddRead(fd.GetFieldName(), fd.GetValue<double>())
            : msg.SetRead(fd.GetFieldName(), fd.GetValue<double>());
        break;
    case FiledVal::Float:
        msg.IsQueryList() ? msg.AddRead(fd.GetFieldName(), fd.GetValue<float>())
            : msg.SetRead(fd.GetFieldName(), fd.GetValue<float>());
        break;
    case FiledVal::String:
        msg.IsQueryList() ? msg.AddRead(fd.GetFieldName(), string((char*)fd.GetBuff(), fd.GetValidLen()))
            : msg.SetRead(fd.GetFieldName(), string((char*)fd.GetBuff(), fd.GetValidLen()));
        break;
    case FiledVal::Buff:
        msg.IsQueryList() ? msg.AddRead(fd.GetFieldName(), Variant(fd.GetValidLen(), (char*)fd.GetBuff()))
            : msg.SetRead(fd.GetFieldName(), Variant(fd.GetValidLen(), (char*)fd.GetBuff()));
        break;
    }
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

bool DBManager::InitManager(void)
{
    return false;
}

int DBManager::GetObjectType() const
{
    return IObject::DBMySql;
}

bool DBManager::PrcsPublicMsg()
{
    return true;
}

IObject *DBManager::PrcsNotObjectReceive(ISocket *, const char *, int)
{
    return NULL;
}

void DBManager::LoadConfig()
{
    TiXmlDocument doc;
    doc.LoadFile("DBManager.xml");

    const TiXmlElement *rootElement = doc.RootElement();
    const TiXmlNode *node = rootElement ? rootElement->FirstChild("DBManager") : NULL;
    const TiXmlElement *cfg = node ? node->ToElement() : NULL;
    if (!cfg)
        return;

    const char *tmp = cfg->Attribute("thread");
    int n = tmp ? (int)Utility::str2int(tmp) : 1;
    tmp = cfg->Attribute("buff");
    InitThread(n, 0);

    const TiXmlNode *dbNode = node ? node->FirstChild("ObjectDB") : NULL;
    while (dbNode)
    {
        if (ObjectDB *db = ObjectDB::ParseMySql(*dbNode->ToElement()))
            AddObject(db);

        dbNode = dbNode->NextSibling("ObjectDB");
    }
}

DECLARE_MANAGER_ITEM(DBManager)