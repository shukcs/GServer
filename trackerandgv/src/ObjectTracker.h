#ifndef  __OBJECT_UAV_H__
#define __OBJECT_UAV_H__

#include "ObjectAbsPB.h"
#include <stdio.h>

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

    void SetSimId(const std::string &sim);
public:
    static int TrackerType();
protected:
    virtual int GetObjectType()const;
    virtual void ProcessMessage(IMessage *msg);
    void PrcsProtoBuff(uint64_t);

    void CheckTimer(uint64_t ms);
    void OnConnected(bool bConnected);
    void InitObject();
    void _respondLogin(int seq, int res);
    void OnLogined(bool suc, ISocket *s = NULL);
    bool IsAllowRelease()const;
    ILink *GetLink();
private:
    void _prcsPosAuth(das::proto::RequestPositionAuthentication *msg);
    void _prcsOperationInformation(das::proto::PostOperationInformation *msg, uint64_t ms);
    void _prcsAckQueryParameters(das::proto::AckQueryParameters *msg);
    void _prcsAckConfigurParameters(das::proto::AckConfigurParameters *msg);
    void _prcsProgramUpgrade(das::proto::RequestProgramUpgrade *msg);
    int _checkPos(double lat, double lon, double alt);
private:
    friend class TrackerManager;
    std::string                     m_strSim;
    double                          m_lat;
    double                          m_lon;
    FILE*                           m_posRecord;
    int64_t                         m_tmLast;
    std::string                     m_strFile;
};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif//__OBJECT_UAV_H__

