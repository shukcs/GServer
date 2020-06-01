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
        class PostOperationInformation;
        class RequestTrackerIdentityAuthentication;
        class RequestProgramUpgrade;
    }
}

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

class ProtoMsg;
class ObjectGS;
class ObjectUav;
class GV2TrackerMessage;

class TrackerManager : public AbsPBManager
{
public:
    TrackerManager();
    ~TrackerManager();
public:
    bool CanFlight(double lat, double lon, double alt);
public:
    static uint32_t toIntID(const std::string &uavid);
protected:
    int GetObjectType()const;
    IObject *PrcsProtoBuff(ISocket *s);
    bool PrcsPublicMsg(const IMessage &msg);
    void LoadConfig();
    bool IsHasReuest(const char *buf, int len)const;
private:
    IObject *_checkLogin(ISocket *s, const das::proto::RequestTrackerIdentityAuthentication &uia);
    IObject *_checkProgram(ISocket *s, const das::proto::RequestProgramUpgrade &rpu);
private:
};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif//__UAV_MANAGER_H__

