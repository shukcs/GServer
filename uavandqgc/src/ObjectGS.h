#ifndef  __OBJECT_GS_H__
#define __OBJECT_GS_H__

#include "ObjectAbsPB.h"

namespace das {
    namespace proto {
        class RequestGSIdentityAuthentication;
        class RequestNewGS;
        class PostHeartBeat;
        class PostProgram;
        class RequestUavStatus;
        class RequestBindUav;
        class PostParcelDescription;
        class RequestParcelDescriptions;
        class DeleteParcelDescription;
        class OperationDescription;
        class PostOperationDescription;
        class RequestOperationDescriptions;
        class DeleteOperationDescription;
        class PostOperationRoute;
        class RequestUavMission;
        class RequestUavMissionAcreage;
        class RequestMissionSuspend;
        class PostControl2Uav;
        class ParcelDescription;
        class ParcelContracter;
        class RequestIdentityAllocation;
        class GroundStationsMessage;
        class RequestFriends;
        class SyncDeviceList;
        class RequestOperationAssist;
        class RequestABPoint;
        class RequestOperationReturn;
    }
}
class FWItem;
#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

class ProtoMsg;
class Gs2GsMessage;
class DBMessage;
class ObjectGS : public ObjectAbsPB
{
	typedef void (ObjectGS::*ProtobufPrcsF)(const google::protobuf::Message *);
public:
    ObjectGS(const std::string &id, int seq=-1);
    ~ObjectGS();

    void SetPswd(const std::string &pswd);
    void SetCheck(const std::string &str);
    const std::string &GetPswd()const;
    void SetAuth(int);
    int Authorize()const;
    bool GetAuth(AuthorizeType auth = Type_Common)const;
public:
    static int GSType();
protected:
    void OnConnected(bool bConnected)override;
    int GetObjectType()const override;
    void ProcessMessage(const IMessage *msg)override;
    void InitObject()override;
    void CheckTimer(uint64_t ms)override;
    bool IsAllowRelease()const override;
    void RefreshRcv(int64_t ms)override;
    bool ProcessRcvPack(const void *pack)override;

    void processGs2Gs(const google::protobuf::Message &msg, int tp);
    void processControlUser(const google::protobuf::Message &msg);
    void processBind(const DBMessage &msg);
    void processUavsInfo(const DBMessage &msg);
    void processSuspend(const DBMessage &msg);
    void processGSInfo(const DBMessage &msg);
    void processCheckGS(const DBMessage &msg);
    void processPostLandRslt(const DBMessage &msg);
    void processPostPlanRslt(const DBMessage &msg);
    void processCountLandRslt(const DBMessage &msg);
    void processCountPlanRslt(const DBMessage &msg);
    void processDeleteLandRslt(const DBMessage &msg);
    void processDeletePlanRslt(const DBMessage &msg);
    void processQueryPlans(const DBMessage &msg);
    void processFriends(const DBMessage &msg);
    void processQueryLands(const DBMessage &msg);
    void processGSInsert(const DBMessage &msg);
    void processMissions(const DBMessage &msg);
    void processMissionsAcreage(const DBMessage &msg);
private:
    void _prcsLogin(das::proto::RequestGSIdentityAuthentication *msg);
    void _prcsHeartBeat(das::proto::PostHeartBeat *msg);
    void _prcsProgram(das::proto::PostProgram *msg);
    void _prcsReqUavs(das::proto::RequestUavStatus *msg);
    void _prcsSyncDeviceList(das::proto::SyncDeviceList *ms);
    void _prcsReqBind(const das::proto::RequestBindUav &msg);
    void _prcsControl2Uav(das::proto::PostControl2Uav *msg);
    void _prcsPostLand(das::proto::PostParcelDescription *msg);
    void _prcsReqLand(das::proto::RequestParcelDescriptions *msg);
    void _prcsDeleteLand(das::proto::DeleteParcelDescription *msg);
    void _prcsUavIDAllication(das::proto::RequestIdentityAllocation *msg);
    void _prcsPostPlan(das::proto::PostOperationDescription *msg);
    void _prcsReqPlan(das::proto::RequestOperationDescriptions *msg);
    void _prcsDeletePlan(das::proto::DeleteOperationDescription *msg);
    void _prcsPostMission(das::proto::PostOperationRoute *msg);
    void _prcsReqNewGs(das::proto::RequestNewGS *msg);
    void _prcsGsMessage(das::proto::GroundStationsMessage *msg);
    void _prcsReqFriends(das::proto::RequestFriends *msg);
    void _prcsReqMissons(das::proto::RequestUavMission &msg);
    void _prcsReqMissonsAcreage(das::proto::RequestUavMissionAcreage &msg);
    void _prcsReqSuspend(das::proto::RequestMissionSuspend &msg);
    void _prcsReqAssists(das::proto::RequestOperationAssist *msg);
    void _prcsReqABPoint(das::proto::RequestABPoint *msg);
    void _prcsReqReturn(das::proto::RequestOperationReturn *msg);
private:
    void _checkGS(const std::string &user, int ack);
	void _queryGSData();
    void initFriend();
    void addDBFriend(const std::string &user1, const std::string &user2);
    int _addDatabaseUser(const std::string &user, const std::string &pswd, int seq);
    void notifyUavNewFw(const std::string &fw, int seq);
    void ackSyncDeviceis();
    void saveBind(const std::string &uav, bool bBind);
	static ProtobufPrcsF getProtobufPrcsFunc(const std::string &name);
private:
    friend class GSManager;
    int             m_auth;
    bool            m_bInitFriends;
    int             m_countLand;
    int             m_countPlan;
    int             m_seq;
	int64_t			m_tmLastInfo;
    std::string     m_pswd;
    std::string     m_check;
    std::list<std::string> m_friends;
};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif // __OBJECT_UAV_H__

