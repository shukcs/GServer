#ifndef  __OBJECT_UAV_H__
#define __OBJECT_UAV_H__

#include "ObjectAbsPB.h"

namespace das {
    namespace proto {
        class RequestBindUav;
        class PostOperationInformation;
        class RequestMissionSuspend;
        class RequestPositionAuthentication;
        class AckQueryParameters;
        class AckConfigurParameters;
        class RequestProgramUpgrade;
    }
}

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

class ObjectGS;
class Variant;
class GV2TrackerMessage;
class ObjectTracker : public ObjectAbsPB
{
private:
    typedef struct _RidgeDat{
        int     idx;
        double  length;
    } RidgeDat;
public:
    ObjectTracker(const std::string &id, const std::string &sim="");
    ~ObjectTracker();

    bool IsValid()const;
    void SetValideTime(int64_t tmV);
    void SetSimId(const std::string &sim);
public:
    static int TrackerType();
protected:
    virtual int GetObjectType()const;
    virtual void ProcessMessage(IMessage *msg);
    void PrcsProtoBuff();

    void CheckTimer(uint64_t ms);
    void OnConnected(bool bConnected);
    void InitObject();
    void _respondLogin(int seq, int res);
    void OnLogined(bool suc, ISocket *s = NULL);
    bool IsAllowRelease()const;
    ILink *GetLink();
private:
    void _prcsPosAuth(das::proto::RequestPositionAuthentication *msg);
    void _prcsOperationInformation(das::proto::PostOperationInformation *msg);
    void _prcsAckQueryParameters(das::proto::AckQueryParameters *msg);
    void _prcsAckConfigurParameters(das::proto::AckConfigurParameters *msg);
    void _prcsProgramUpgrade(das::proto::RequestProgramUpgrade *msg);
    int _checkPos(double lat, double lon, double alt);
private:
    friend class TrackerManager;
    std::string                     m_strSim;
    double                          m_lat;
    double                          m_lon;
    int64_t                         m_tmValidLast;
    das::proto::AckQueryParameters  *m_params;
};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif//__OBJECT_UAV_H__

