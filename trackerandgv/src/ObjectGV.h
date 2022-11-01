#ifndef  __OBJECT_GS_H__
#define __OBJECT_GS_H__

#include "ObjectAbsPB.h"

namespace das {
    namespace proto {
        class RequestGVIdentityAuthentication;
        class PostHeartBeat;
        class PostProgram;
        class QueryParameters;
        class ConfigureParameters;
        class RequestIdentityAllocation;
        class SyncDeviceList;
    }
}
class FWItem;
#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

class ProtoMsg;
class TiXmlElement;

class ObjectGV : public ObjectAbsPB
{
public:
    enum GVAuthorizeType
    {
        Type_Common = 1,
        Type_TrackerManager = Type_Common << 1,
        Type_Admin = Type_TrackerManager << 1,
        Type_ALL = Type_Common | Type_TrackerManager | Type_Admin,
    };
public:
    ObjectGV(const std::string &id);
    ~ObjectGV();

    void SetPswd(const std::string &pswd);
    const std::string &GetPswd()const;
    void SetAuth(int);
    int Authorize()const;
    void SetSeq(int seq);
public:
    static int GVType();
    static ObjectGV *ParseObjecy(const TiXmlElement &);
protected:
    void OnConnected(bool bConnected);
    int GetObjectType()const;
    void ProcessMessage(const IMessage *msg);
    void PrcsProtoBuff(uint64_t);

    void process2GsMsg(const google::protobuf::Message *msg);
    void ackSyncDeviceis();
    void processEvent(const IMessage &msg, int tp);
    void InitObject();
    void CheckTimer(uint64_t ms)override;
    bool IsAllowRelease()const;
    void FreshLogin(uint64_t ms);
private:
    void _prcsLogin(das::proto::RequestGVIdentityAuthentication *msg);
    void _prcsHeartBeat(das::proto::PostHeartBeat *msg);
    void _prcsSyncDevice(das::proto::SyncDeviceList *ms);
    void _prcsQueryParameters(das::proto::QueryParameters *pb);
    void _prcsConfigureParameters(das::proto::ConfigureParameters *pb);
private:
    friend class GVManager;
    int             m_auth;
    std::string     m_pswd;
    int             m_seq;
    uint64_t        m_tmLastInfo;
};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif // __OBJECT_UAV_H__

