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
        class QueryParameters;
        class RequestTrackerIdentityAuthentication;
        class Request3rdIdentityAuthentication;
        class PostHeartBeat;
    }
}

#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
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

    void CheckTimer(uint64_t ms, char *buf, int len);
    void OnConnected(bool bConnected);
    void InitObject();
    void _respondLogin(const das::proto::RequestTrackerIdentityAuthentication &ra);
    void _respond3rdLogin(const das::proto::Request3rdIdentityAuthentication &ra);
    void OnLogined(bool suc, ISocket *s = NULL);
    bool IsAllowRelease()const;
    ILink *GetLink();
    void FreshLogin(uint64_t ms);
private:
    void _prcsPosAuth(das::proto::RequestPositionAuthentication *msg);
    void _prcsOperationInformation(das::proto::PostOperationInformation *msg, uint64_t ms);
    void _prcsAckQueryParameters(das::proto::AckQueryParameters *msg);
    void _prcsAckConfigurParameters(das::proto::AckConfigurParameters *msg);
    void _prcsProgramUpgrade(das::proto::RequestProgramUpgrade *msg);
    void _prcsHeartBeat(const das::proto::PostHeartBeat &msg);
    int _checkPos(double lat, double lon, double alt);
    void _checkFile();
    void _ackPartParameters(const std::string &gv, const das::proto::QueryParameters &qp);
private:
    friend class TrackerManager;
    std::string                     m_strSim;
    double                          m_lat;
    double                          m_lon;
    FILE*                           m_posRecord;
    int64_t                         m_tmLast;
    int64_t                         m_tmPos;
    int                             m_statGX;
    std::string                     m_strFile;
};

#endif//__OBJECT_UAV_H__

