#ifndef __VG_TRIGGER_H__
#define __VG_TRIGGER_H__

#include <string>

class ExecutItem;
class TiXmlElement;
class VGTrigger
{
    enum SqlOrder
    {
        Order_None,
        Order_Follows,
        Order_Precedes,
    };
public:
    const std::string &GetName()const;
    bool IsValid()const;

    std::string ToSqlString()const;
public:
    static VGTrigger *Parse(const TiXmlElement &e);
protected:
    VGTrigger(const std::string &name);
    virtual ~VGTrigger() {}

    void SetTable(const std::string &tb);
    void SetEvent(int evt);
    void SetExecuteBeforeEvent(bool b);
    void SetExecuteItem(ExecutItem *i);
    void SetOrder(SqlOrder);
private:
	std::string			m_name;
    int                 m_event;
    bool                m_bBefore;
    SqlOrder            m_order;
    ExecutItem          *m_execItem;
    std::string			m_eventTable;
};
#endif // __VG_TRIGGER_H__