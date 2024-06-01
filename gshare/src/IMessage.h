#ifndef __IMESSAGE_H__
#define __IMESSAGE_H__

#include <string>
#include "stdconfig.h"

#define SymbolString(sb) #sb
#define CLASS_INFO(sb) \
protected: \
virtual const std::string &_className()const { \
    static std::string ret(SymbolString(sb)); \
    return ret; \
}
#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

class ILink;
class IObject;
class IObjectManager;

class MessageData
{
public:
    SHARED_DECL MessageData(const IObject &sender, int16_t tpMs);
    SHARED_DECL MessageData(const IObjectManager *sender, int16_t tpMs);
    SHARED_DECL MessageData(const std::string &sender, int tpSnd, int16_t tpMs);
    SHARED_DECL virtual ~MessageData();

    SHARED_DECL bool IsValid() const;
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
        Authentication,
        BindUav,
        PostOR,
        ControlDevice,
        ControlDevice2,
        SychMission,
        QueryDevice,
        DeviceAllocation,
        NotifyFWUpdate,
        SyncDeviceis,
        UserMessageEnd,
        ReqService,

        BindUavRslt,
        ControlDeviceRslt,
        SychMissionRslt,
        PostORRslt,
        PushUavSndInfo,
        ControlUser,
        QueryDeviceRslt,
        SyncDeviceisRslt,
        DeviceAllocationRslt,
        ReqServiceRslt,
        ServiceData,

        DeviceMessageEnd,
        User2User = DeviceMessageEnd,
        User2UserAck,

        User2UserEnd,
        MSGDBBeging = User2UserEnd,
        DBExec,
        DBAckBeging,
        DeviceQueryRslt = DBAckBeging,
        DeviceisQueryRslt,
        DeviceUpdatePosRslt,
        DeviceBindRslt,
        DeviceisMaxIDRslt,
        MisionUpdateRslt,
        MisionQueryRslt,
        UserInsertRslt,
        UserQueryRslt,
        UserCheckRslt,
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
        QueryMissionsRslt,
        QueryMissionsAcreageRslt,
        SuspendRslt,
        UpdateSWKeysRslt,
        SWRegistRslt,
        QuerySWKeyInfoRslt,
        InserFZRslt,
        QueryFZRslt,

        DBAckEnd,
        DBMsgEnd,
        GXClinetStat,

        SendMail,
        MailRslt,

        UserMsg,
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
    SHARED_DECL std::string ClassName()const;
    int CreateThreadID()const;
protected:
    virtual const std::string &_className()const = 0;
protected:
    MessageData *m_data;
    int32_t     m_tpRcv;
    std::string m_idRcv;
};

class ObjectSignal : public IMessage
{
    CLASS_INFO(ObjectSignal)
public:
    enum EventType {
        S_Release = UserMsg + 0x2000,
        S_Login,
        S_Logout,
    };
public:
    SHARED_DECL ObjectSignal(const IObject *sender, int32_t rcvTp, int e = S_Release, const std::string &rcv=std::string());
protected:
    void *GetContent()const;
    int GetContentLength()const;
};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif //__IMESSAGE_H__