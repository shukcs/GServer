#ifndef  __OBJECT_GS_H__
#define __OBJECT_GS_H__

#include "ObjectAbsPB.h"

namespace das {
    namespace proto {
        class RequestFZUserIdentity;
        class RequestNewFZUser;
        class SyncFZUserList;
        class PostHeartBeat;
        class FZUserMessage;
        class RequestFriends;
        class AddSWKey;
        class SWRegist;
        class PostFZResult;
        class RequestFZResults;
        class FZInfo;
        class PostFZInfo;
        class RequestFZInfo;
        class PostGetFZPswd;
        class PostChangeFZPswd;
    }
}
class FWItem;
#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

class ProtoMsg;
class FZ2FZMessage;
class DBMessage;
class MailRsltMessage;
class ObjectVgFZ : public ObjectAbsPB
{
public:
    ObjectVgFZ(const std::string &id, int seq=-1);
    ~ObjectVgFZ();

    void SetPswd(const std::string &pswd);
    void SetCheck(const std::string &str);
    void SetPcsn(const std::string &str);
    const std::string &GetPCSn()const;
    const std::string &GetPswd()const;
    void SetAuth(int);
    int Authorize()const;
    int GetVer()const;
    bool GetAuth(AuthorizeType auth = Type_Common)const;
public:
    static int FZType();
    static bool CheckSWKey(const std::string &swk);
protected:
    void OnConnected(bool bConnected);
    int GetObjectType()const;
    void ProcessMessage(const IMessage *msg);
    void PrcsProtoBuff(uint64_t);

    void processFZ2FZ(const google::protobuf::Message &msg, int tp);
    void processFZInfo(const DBMessage &msg);
    void processFZInsert(const DBMessage &msg);
    void processCheckUser(const DBMessage &msg);
    void processFriends(const DBMessage &msg);
    void processInserSWSN(const DBMessage &msg);
    void processSWRegist(const DBMessage &msg);
    void processAckFZRslt(const DBMessage &msg);
    void processAckFZRslts(const DBMessage &msg);
    void processMailRslt(const MailRsltMessage &msg);

    bool ackLogin(const DBMessage &msg);
    void forGetPswd(const DBMessage &msg);
    void InitObject();
    void CheckTimer(uint64_t ms, char *buf, int len);
    bool IsAllowRelease()const;
    void FreshLogin(uint64_t ms);
    bool CheckMail(const std::string &str);
private:
    void _prcsLogin(das::proto::RequestFZUserIdentity *msg);
    void _prcsHeartBeat(das::proto::PostHeartBeat *msg);
    void _prcsSyncDeviceList(das::proto::SyncFZUserList *ms);
    void _prcsReqNewFz(das::proto::RequestNewFZUser *msg);
    void _prcsFzMessage(das::proto::FZUserMessage *msg);
    void _prcsReqFriends(das::proto::RequestFriends *msg);
    void _prcsAddSWKey(das::proto::AddSWKey *msg);
    void _prcsSWRegist(das::proto::SWRegist *msg);
    void _prcsPostFZResult(das::proto::PostFZResult *msg);
    void _prcsRequestFZResults(const das::proto::RequestFZResults &msg);
    void _prcsPostFZInfo(const das::proto::PostFZInfo &msg);
    void _prcsRequestFZInfo(const das::proto::RequestFZInfo &msg);
    void _prcsPostGetFZPswd(const das::proto::PostGetFZPswd &msg);
    void _prcsPostChangeFZPswd(const das::proto::PostChangeFZPswd &msg);
private:
    bool _saveInfo(const das::proto::FZInfo &info);
    void _checkFZ(const std::string &user, int ack);
    void initFriend();
    void addDBFriend(const std::string &user1, const std::string &user2);
    int _addDatabaseUser(const std::string &user, const std::string &pswd, int seq);
    void ackSyncDeviceis();
private:
    friend class VgFZManager;
    int             m_auth;
    bool            m_bInitFriends;
    int             m_seq;
    int             m_ver;
    uint64_t        m_tmLastInfo;
    std::string     m_pswd;
    std::string     m_check;
    std::string     m_pcsn;
    std::string     m_email;
    std::string     m_info;
    std::list<std::string> m_friends;
};

#endif // __OBJECT_UAV_H__

