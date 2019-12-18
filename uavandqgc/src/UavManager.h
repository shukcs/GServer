#ifndef  __UAV_MANAGER_H__
#define __UAV_MANAGER_H__

#include "ObjectBase.h"

namespace google {
    namespace protobuf {
        class Message;
    }
}
class TiXmlDocument;
namespace das {
    namespace proto {
        class RequestBindUav;
        class PostOperationInformation;
        class PostStatus2GroundStation;
        class PostControl2Uav;
        class RequestUavIdentityAuthentication;
        class RequestUavStatus;
        class AckRequestUavStatus;
    }
}

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

class ProtoMsg;
class ObjectGS;
class ObjectUav;
class DBMessage;

class UavManager : public IObjectManager
{
public:
    UavManager();
    ~UavManager();
public:
    bool CanFlight(double lat, double lon, double alt);
public:
    static uint32_t toIntID(const std::string &uavid);
protected:
    int GetObjectType()const;
    IObject *PrcsNotObjectReceive(ISocket *s, const char *buf, int len);
    bool PrcsPublicMsg(const IMessage &msg);
    void LoadConfig();
    bool InitManager();
private:
    void _getLastId();

    void sendBindAck(const ObjectUav &uav, int ack, int res, bool bind, const std::string &gs);
    IObject *_checkLogin(ISocket *s, const das::proto::RequestUavIdentityAuthentication &uia);
    void _checkBindUav(const das::proto::RequestBindUav &rbu, ObjectGS *gs);
    void _checkUavInfo(const das::proto::RequestUavStatus &uia, ObjectGS *gs);
    void processAllocationUav(int seqno, const std::string &id);
    void processMaxID(const DBMessage &msg);
    void addUavId(int seq, const std::string &uav);
    void queryUavInfo(ObjectGS *gs, int seq, const std::list<std::string> &uavs);
    void saveBind(const std::string &uav, bool bBind, ObjectGS *gs);
private:
    ProtoMsg    *m_p;
    uint32_t    m_lastId;
    bool        m_bInit;
};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif//__UAV_MANAGER_H__

