#ifndef __IMESSAGE_H__
#define __IMESSAGE_H__

#include <string>
#include "stdconfig.h"

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

class ILink;
class IObject;
class IObjectManager;

class MessageData
{
public:
    SHARED_DECL MessageData(IObject *sender, int16_t tpMs);
    SHARED_DECL MessageData(IObjectManager *sender, int16_t tpMs);
    SHARED_DECL MessageData(const std::string &sender, int tpSnd, int16_t tpMs);
    SHARED_DECL virtual ~MessageData();
private:
    bool IsValid() const;
private:
    friend class IMessage;
    int         m_threadID;
    int16_t     m_tpMsg;
    int16_t     m_tpSender;
    std::string m_idSnd;
};

/*************************************************************************
这是个连接实体之间通讯消息抽象类，ObjectManagers::SendMsg()发送到接受实体
IMessage(sender, id, rcv, tpMsg)
sender:发送对象，可以是IObject或IObjectManager,但必须属于一个线程
id:接收IObject ID，可以空，空代表接收对象是IObjectManager
rcv:接收对象类型
tpMsg:消息类型
**************************************************************************/
class IMessage
{
public:
    enum MessageType
    {
        Unknown,
        BindUav,
        PostOR,
        ControlUav,
        SychMission,
        QueryUav,
        UavAllocation,
        NotifyFWUpdate,
        Gs2UavEnd,

        BindUavRes,
        ControlUavRes,
        SychMissionRes,
        PostORRes,
        PushUavSndInfo,
        ControlGs,
        QueryUavRes,
        UavAllocationRes,

        Uav2GsEnd,
        Gs2GsBeging = Uav2GsEnd,
        Gs2GsMsg,
        Gs2GsAck,

        Gs2GsEnd,
        MSGDBBeging = Gs2GsEnd,
        DBExec,
        DBAckBeging,
        UavQueryRslt = DBAckBeging,
        UavsQueryRslt,
        UavUpdatePosRslt,
        UavBindRslt,
        UavsMaxIDRslt,
        MisionUpdateRslt,
        MisionQueryRslt,
        GSInsertRslt,
        GSQueryRslt,
        GSCheckRslt,
        FriendUpdateRslt,
        FriendQueryRslt,
        LandInsertRslt,
        LandQueryRslt,
        CountLandRslt,
        CountPlanRslt,
        PlanInsertRslt,
        PlanQueryRslt,
        DeleteLandRslt,
        DeletePlanRslt,
        QueryMissionLandRslt,
        QueryMissionsRslt,

        DBAckEnd,

        DBMsgEnd,

        User,
    };
public:
    SHARED_DECL IMessage(MessageData *data, const std::string &rcv, int32_t tpRcv);
    SHARED_DECL virtual ~IMessage();

    virtual void *GetContent()const = 0;
    virtual int GetContentLength()const = 0;

    SHARED_DECL int32_t GetReceiverType()const;
    SHARED_DECL void SetMessgeType(int tp);
    SHARED_DECL int GetMessgeType()const;
    SHARED_DECL const std::string &GetReceiverID()const;
    SHARED_DECL int32_t GetSenderType()const;
    SHARED_DECL const std::string &GetSenderID()const;

    SHARED_DECL bool IsValid()const;
    int CreateThreadID()const;
protected:
    MessageData *m_data;
    int32_t     m_tpRcv;
    std::string m_idRcv;
};

class ObjectEvent : public IMessage
{
public:
    enum EventType {
        E_Release = User + 0x2000,
    };
public:
    ObjectEvent(const std::string &rcv, int32_t rcvTp, EventType e=E_Release);

protected:
    void *GetContent()const;
    int GetContentLength()const;
private:
    EventType m_tp;
};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif //__IMESSAGE_H__