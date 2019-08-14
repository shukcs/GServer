#ifndef  __UAV_MANAGER_H__
#define __UAV_MANAGER_H__

#include "ObjectBase.h"

namespace google {
    namespace protobuf {
        class Message;
    }
}

namespace das {
    namespace proto {
        class RequestBindUav;
        class PostOperationInformation;
        class PostStatus2GroundStation;
        class PostControl2Uav;
        class RequestUavIdentityAuthentication;
        class RequestUavStatus;
        class UavStatus;
    }
}
class VGMySql;
class ExecutItem;

#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

class ProtoMsg;
class ObjectUav : public IObject
{
public:
    bool IsConnect()const;
protected:
    ObjectUav(const std::string &id);
    ~ObjectUav();

    void InitBySqlResult(const ExecutItem &sql);
    void transUavStatus(das::proto::UavStatus &us);

    void OnConnected(bool bConnected);
    virtual int GetObjectType()const;
    virtual void ProcessMassage(const IMessage &msg);
    virtual int ProcessReceive(void *buf, int len);
    virtual int GetSenLength()const;
    virtual int CopySend(char *buf, int sz, unsigned form = 0);
    virtual void SetSended(int sended = -1);//-1,·¢ËÍÍê
    void RespondLogin(int seq, int res);
protected:
    static void AckControl2Uav(const das::proto::PostControl2Uav &msg, ObjectUav *obj, int res);
private:
    void prcsRcvPostOperationInfo(das::proto::PostOperationInformation *msg);
    void prcsRcvPost2Gs(das::proto::PostStatus2GroundStation *msg);
    void processBind(das::proto::RequestBindUav *msg);
    void processControl2Uav(das::proto::PostControl2Uav *msg);

    void _send(const google::protobuf::Message &msg);
private:
    friend class UavManager;
    bool        m_bBind;
    bool        m_bConnected;
    double      m_lat;
    double      m_lon;
    int64_t     m_tmLastInfo;
    int64_t     m_tmLastBind;
    ProtoMsg    *m_p;
    std::string m_lastBinder;
};

class UavManager : public IObjectManager
{
public:
    UavManager();
    ~UavManager();
public:
    int PrcsBind(const das::proto::RequestBindUav *msg, const std::string &gsOld);
    void UpdatePos(const std::string &uav, double lat, double lon);
protected:
    int GetObjectType()const;
    IObject *ProcessReceive(ISocket *s, const char *buf, int &len);
    bool PrcsRemainMsg(const IMessage &msg);
private:
    void _ensureDBValid();

    void sendBindRes(const das::proto::RequestBindUav &msg, int res, bool bind);
    IObject *_checkLogin(ISocket *s, const das::proto::RequestUavIdentityAuthentication &uia);
    void _checkBindUav(const das::proto::RequestBindUav &rbu);
    void _checkUavInfo(const das::proto::RequestUavStatus &uia, const std::string &gs);
private:
    VGMySql *m_sqlEng;
    ProtoMsg *m_p;
};

#endif//__UAV_MANAGER_H__

