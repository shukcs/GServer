#include "VGTrigger.h"
#include "DBExecItem.h"
#include "MysqlDB.h"
#include "VGMysql.h"
#include "tinyxml.h"
#include <stdio.h>
#include <string.h>

using namespace std;
static const char *sTriggerFmt = "CREATE TRIGGER %s %s %s ON %s FOR EACH ROW";

VGTrigger::VGTrigger(const std::string &name) :m_name(name)
, m_event(ExecutItem::Unknown), m_bBefore(false)
{
}

VGTrigger::~VGTrigger()
{
    for (ExecutItem *itr : m_execItems)
    {
        if (itr && !itr->IsRef())
            delete itr;
    }
}

const std::string &VGTrigger::GetName() const
{
    return m_name;
}

string VGTrigger::ToSqlString() const
{
    if (!IsValid())
        return string();

    string strTime = _timeStr();
    string strEvt = _eventStr();

    string ret;
    int l = strlen(sTriggerFmt);
    l += strEvt.length() + strTime.length() + m_name.length() + m_eventTable.length();
    ret.resize(l);
    memset(&ret.at(0), 0, l);
    sprintf(&ret.at(0), sTriggerFmt, m_name.c_str()
        , strTime.c_str(), strEvt.c_str(), m_eventTable.c_str());

    if (m_execItems.size() == 1)
        return string(ret.c_str()) + " " + m_execItems.front()->GetSqlString();

    ret += string(ret.c_str())+" BEGIN";
    for (ExecutItem *itr : m_execItems)
    {
        ret += " " + itr->GetSqlString() + ";";
    }
    ret += " END";
    return ret;
}

bool VGTrigger::IsValid() const
{
    return m_event >= ExecutItem::Insert
        && m_event < ExecutItem::Select
        && m_execItems.size() > 0
        && m_eventTable.length()>0;
}

void VGTrigger::SetTable(const std::string &tb)
{
    m_eventTable = tb;
}

void VGTrigger::SetEvent(int evt)
{
    if (evt >= ExecutItem::Insert && evt < ExecutItem::Select)
        m_event = evt;
    else
        m_event = ExecutItem::Unknown;
}

void VGTrigger::SetExecuteBeforeEvent(bool b)
{
    m_bBefore = b;
}

bool VGTrigger::AddExecuteItem(ExecutItem *i)
{
    if (i && !VGMySql::IsContainsInList(m_execItems, i))
    {
        m_execItems.push_back(i);
        return true;
    }
    return false;
}

string VGTrigger::_timeStr() const
{
    return m_bBefore ? "BEFORE" : "AFTER";
}

string VGTrigger::_eventStr() const
{
    switch (m_event)
    {
    case ExecutItem::Insert:
        return "INSERT";
    case ExecutItem::Delete:
        return "DELETE";
    case ExecutItem::Update:
        return "UPDATE";
    default:
        break;
    }
    return string();
}

void VGTrigger::_addSqls(const list<string> &items, const MysqlDB &db)
{
    for (const string &itr : items)
    {
        ExecutItem *sql= db.GetSqlByName(itr);
        if (sql && AddExecuteItem(sql))
            sql->SetRef(true);     
    }
}

void VGTrigger::_parseSqls(const TiXmlElement &node, const MysqlDB &db)
{
    const TiXmlNode *tgNode = node.FirstChild("SQL");
    while (tgNode)
    {
        if (ExecutItem *trg = ExecutItem::parse(tgNode->ToElement(), db))
            AddExecuteItem(trg);

        tgNode = tgNode->NextSibling("SQL");
    }
}

VGTrigger *VGTrigger::Parse(const TiXmlNode &node, const MysqlDB &db)
{
    TiXmlElement e = *node.ToElement();
    const char *tmp = e.Attribute("name");
    string name = tmp ? tmp : string();

    tmp = e.Attribute("table");
    string table = tmp ? tmp : string();
    ExecutItem::ExecutType evt = ExecutItem::transToSqlType(e.Attribute("event"));
    if (name.empty() || table.empty() || evt < ExecutItem::Insert || evt >= ExecutItem::Select)
        return NULL;

    if (VGTrigger *ret = new VGTrigger(name))
    {
        tmp = e.Attribute("before");
        ret->m_bBefore = tmp ? 0 == strnicmp(tmp, "false", 6) : false;
        ret->m_eventTable = table;
        ret->m_event = evt;

        if (const char *tmpRef = e.Attribute("refsql"))
            ret->_addSqls(VGMySql::SplitString(tmpRef, ";"), db);
        else
            ret->_parseSqls(e, db);

        if (ret->IsValid())
            return ret;

        delete ret;
    }
    return NULL;
}
