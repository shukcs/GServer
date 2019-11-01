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
class VGMySql;
class ExecutItem;

#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

class ProtoMsg;
class ObjectGS;
class UavManager : public IObjectManager
{
public:
    UavManager();
    ~UavManager();
public:
    int PrcsBind(const das::proto::RequestBindUav *msg, const std::string &gsOld, bool binded, ObjectGS *sender);
    void UpdatePos(const std::string &uav, double lat, double lon);
    VGMySql *GetMySql()const;
public:
    static uint32_t toIntID(const std::string &uavid);
protected:
    int GetObjectType()const;
    IObject *PrcsReceiveByMgr(ISocket *s, const char *buf, int &len);
    bool PrcsRemainMsg(const IMessage &msg);
    void LoadConfig();
private:
    void _parseMySql(const TiXmlDocument &doc);

    void sendBindRes(const das::proto::RequestBindUav &msg, int res, bool bind);
    IObject *_checkLogin(ISocket *s, const das::proto::RequestUavIdentityAuthentication &uia);
    void _checkBindUav(const das::proto::RequestBindUav &rbu, ObjectGS *gs);
    void _checkUavInfo(const das::proto::RequestUavStatus &uia, ObjectGS *gs);
    void processAllocationUav(int seqno, const std::string &id);
    int _addUavId(const std::string &uav);
    bool _queryUavInfo(das::proto::AckRequestUavStatus &aus, const std::string &);
private:
    VGMySql     *m_sqlEng;
    ProtoMsg    *m_p;
    uint32_t    m_lastId;
    bool        m_bInit;
};

#endif//__UAV_MANAGER_H__

