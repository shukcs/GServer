#ifndef  __OBJECT_GS_H__
#define __OBJECT_GS_H__

#include "ObjectAbsPB.h"

namespace das {
    namespace proto {
        class RequestFZUserIdentity;
        class RequestNewFZUser;
        class SyncFZUserList;
        class AckHeartBeat;
        class FZUserMessage;
        class RequestFriends;
    }
}
class FWItem;
#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

class ProtoMsg;
class FZ2FZMessage;
class DBMessage;
class ObjectVgFZ : public ObjectAbsPB
{
public:
    enum GSAuthorizeType
    {
        Type_Common = 1,
        Type_UavManager = Type_Common << 1,
        Type_Admin = Type_UavManager << 1,
        Type_ALL = Type_Common | Type_UavManager | Type_Admin,
    };
public:
    ObjectVgFZ(const std::string &id, int seq=-1);
    ~ObjectVgFZ();

    void SetPswd(const std::string &pswd);
    void SetCheck(const std::string &str);
    const std::string &GetPswd()const;
    void SetAuth(int);
    int Authorize()const;
    bool GetAuth(GSAuthorizeType auth = Type_Common)const;
public:
    static int FZType();
protected:
    void OnConnected(bool bConnected);
    int GetObjectType()const;
    void ProcessMessage(IMessage *msg);
    void PrcsProtoBuff(uint64_t);

    void processFZ2FZ(const google::protobuf::Message &msg, int tp);
    void processFZInfo(const DBMessage &msg);
    void processFZInsert(const DBMessage &msg);
    void processCheckUser(const DBMessage &msg);
    void processFriends(const DBMessage &msg);

    void InitObject();
    void CheckTimer(uint64_t ms, char *buf, int len);
    bool IsAllowRelease()const;
    void FreshLogin(uint64_t ms);
private:
    void _prcsLogin(das::proto::RequestFZUserIdentity *msg);
    void _prcsHeartBeat(das::proto::AckHeartBeat *msg);
    void _prcsSyncDeviceList(das::proto::SyncFZUserList *ms);
    void _prcsReqNewFz(das::proto::RequestNewFZUser *msg);
    void _prcsFzMessage(das::proto::FZUserMessage *msg);
    void _prcsReqFriends(das::proto::RequestFriends *msg);
private:
    void _checkFZ(const std::string &user, int ack);
    void initFriend();
    void addDBFriend(const std::string &user1, const std::string &user2);
    int _addDatabaseUser(const std::string &user, const std::string &pswd, int seq);
    void ackSyncDeviceis();
private:
    friend class VgFZManager;
    int             m_auth;
    std::string     m_pswd;
    std::string     m_check;
    bool            m_bInitFriends;
    int             m_seq;
    uint64_t        m_tmLastInfo;
    std::list<std::string> m_friends;
};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif // __OBJECT_UAV_H__

