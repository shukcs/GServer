#include "VGTrigger.h"
#include "DBExecItem.h"
#include "VGDBManager.h"
#include "tinyxml.h"

using namespace std;

VGTrigger::VGTrigger(const std::string &name) :m_name(name)
, m_event(ExecutItem::Unknown), m_bBefore(false)
, m_order(Order_None), m_execItem(NULL)
{
}

const std::string &VGTrigger::GetName() const
{
    return m_name;
}

string VGTrigger::ToSqlString() const
{
    return string();
}

bool VGTrigger::IsValid() const
{
    return m_event >= ExecutItem::Insert && m_event < ExecutItem::Select && m_execItem != NULL;
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

void VGTrigger::SetExecuteItem(ExecutItem *i)
{
    m_execItem = i;
}

void VGTrigger::SetOrder(SqlOrder od)
{
    m_order = od;
}

VGTrigger *VGTrigger::Parse(const TiXmlElement &e)
{
    return NULL;
}
