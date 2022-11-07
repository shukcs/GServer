#ifndef  __UAV_MANAGER_H__
#define __UAV_MANAGER_H__

#include "ObjectAbsPB.h"

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
        class NotifyProgram;
    }
}

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

class ProtoMsg;
class ObjectGS;
class ObjectUav;
class DBMessage;
class GS2UavMessage;

class UavManager : public AbsPBManager
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
    IObject *PrcsProtoBuff(ISocket *s, const google::protobuf::Message *proto);
    bool PrcsPublicMsg(const IMessage &msg);
    void LoadConfig(const TiXmlElement *root);
    bool IsHasReuest(const char *buf, int len)const;
private:
    void _getLastId();
    IObject *_checkLogin(ISocket *s, const das::proto::RequestUavIdentityAuthentication &uia);

    void sendBindAck(const ObjectUav &uav, int ack, int res, bool bind, const std::string &gs);
    void checkUavInfo(const das::proto::RequestUavStatus &uia, const GS2UavMessage &gs);
    void processAllocationUav(int seqno, const std::string &id);
    void processNotifyProgram(const das::proto::NotifyProgram &proto);
    void processMaxID(const DBMessage &msg);
    void addUavId(int seq, const std::string &uav);
    void queryUavInfo(const std::string &gs, int seq, const std::list<std::string> &uavs, bool bAdd);
private:
    uint32_t    m_lastId;
};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif//__UAV_MANAGER_H__

