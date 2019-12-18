#ifndef __VG_TRIGGER_H__
#define __VG_TRIGGER_H__

#include <string>
#include <list>

class ExecutItem;
class TiXmlElement;
class TiXmlNode;
class MysqlDB;
class VGTrigger
{
public:
    virtual ~VGTrigger();
    const std::string &GetName()const;
    bool IsValid()const;

    std::string ToSqlString()const;
public:
    static VGTrigger *Parse(const TiXmlNode &e, const MysqlDB &db);
protected:
    VGTrigger(const std::string &name);

    void SetTable(const std::string &tb);
    void SetEvent(int evt);
    void SetExecuteBeforeEvent(bool b);
    bool AddExecuteItem(ExecutItem *i);
private:
    std::string _timeStr()const;
    std::string _eventStr()const;
    void _addSqls(const std::list<std::string> &items, const MysqlDB &db);
    void _parseSqls(const TiXmlElement &node, const MysqlDB &db);
private:
	std::string			    m_name;
    int                     m_event;
    bool                    m_bBefore;
    std::string			    m_eventTable;
    std::list<ExecutItem*>  m_execItems;
};
#endif // __VG_TRIGGER_H__