#include "GXClient.h"
#include "das.pb.h"
#include "Utility.h"
#include "ObjectManagers.h"
#include "GXLink.h"
#include "ObjectUav.h"
#include "Messages.h"
#include "Thread.h"
#include "gsocketmanager.h"
#include "ObjectAbsPB.h"
#include "IMutex.h"
#include "Lock.h"
#include "ProtoMsg.h"

#define GXHost "101.201.234.242"
#define GXPort 8297

#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

using namespace std;
using namespace das::proto;
class GXLinkThread : public Thread
{
public:
    GXLinkThread(ISocketManager *mgr)
        :Thread(true), m_mgr(mgr)
    {
    }
protected:
    bool RunLoop()
    {
        if (!m_mgr)
            return false;

        return m_mgr->Poll(50);
    }
private:
    ISocketManager  *m_mgr;
};
////////////////////////////////////////////////////////////////////////////////
//GXClient
////////////////////////////////////////////////////////////////////////////////
GXClient::GXClient(const std::string &id, GX_Stat st) : m_id(id), m_stat(st)
{
}

GXClient::~GXClient()
{
}

const std::string &GXClient::GetID() const
{
    return m_id;
}

void GXClient::SetStat(GX_Stat st)
{
    if ((m_stat & 0xff) !=st)
        m_stat = st | St_Changed;
}

GXClient::GX_Stat GXClient::GetStat() const
{
    return GX_Stat(m_stat&0xff);
}

bool GXClient::IsChanged() const
{
    return St_Changed == (m_stat & St_Changed);
}

void GXClient::ClearChanged()
{
    m_stat &= ~St_Changed;
}

int GXClient::GXClientType()
{
    return IObject::User + 3;
}
////////////////////////////////////////////////////////////////////////////////
//GXManager
////////////////////////////////////////////////////////////////////////////////
GXManager::GXManager() :IObjectManager()
, m_sockMgr(GSocketManager::CreateManager(0,10000))
, m_parse(new ProtoMsg), m_tmCheck(0)
{
    if (m_sockMgr)
        m_thread = new GXLinkThread(m_sockMgr);
}

GXManager::~GXManager()
{
    delete m_sockMgr;
}

char *GXManager::GetBuff()
{
    return m_bufPublic;
}

int GXManager::BuffLength() const
{
    return 1024;
}

ISocketManager *GXManager::GetSocketManager() const
{
    return m_sockMgr;
}

void GXManager::PushEvent(const std::string &id, GXClient::GX_Stat st)
{
    if (m_mtxM)
    {
        m_mtxM->Lock();
        m_events.push_back(GXEvent(id, st));
        m_mtxM->Unlock();
    }
}

void GXManager::OnRead(GXClientSocket &s)
{
    int len = s.CopyRcv(m_bufPublic, sizeof(m_bufPublic));
    int pos = 0;
    uint32_t tmp = len;
    while (m_parse && len > 18 && m_parse->Parse(m_bufPublic + pos, tmp))
    {
        pos += tmp;
        tmp = len - pos;
        PrcsProtoBuff();
    }
    pos += tmp;
    s.ClearRcv(pos);
}

void GXManager::OnConnect(GXClientSocket *, bool)
{
    PushEvent(string(), GXClient::St_Connect);
}

int GXManager::GetObjectType() const
{
    return GXClient::GXClientType();
}

bool GXManager::PrcsPublicMsg(const IMessage &msg)
{
    int tp = msg.GetMessgeType();
    switch (tp)
    {
    case ObjectSignal::S_Login:
    case ObjectSignal::S_Logout:
        ProcessLogin(msg.GetSenderID(), tp==ObjectSignal::S_Login); break;
    case IMessage::PushUavSndInfo:
        ProcessPostInfo(*(Uav2GXMessage*)&msg); break;
    default:
        break;
    }
    return true;
}

IObject *GXManager::PrcsNotObjectReceive(ISocket *, const char *, int)
{
    return NULL;
}

void GXManager::PrcsProtoBuff()
{
    if (!m_parse)
        return;

    string strMsg = m_parse->GetMsgName();
    if (strMsg == d_p_ClassName(AckUavIdentityAuthentication))
        _prcsLoginAck((AckUavIdentityAuthentication*)m_parse->GetProtoMessage());
    else if (strMsg == d_p_ClassName(AckOperationInformation))
        _prcsInformationAck((AckOperationInformation*)m_parse->GetProtoMessage());
}

void GXManager::LoadConfig()
{
    InitThread(1, 0);
}

bool GXManager::IsReceiveData() const
{
    return false;
}

void GXManager::ProcessLogin(const std::string &id, bool bLogin)
{
    if (id.empty())
        return;

    auto itr = m_objects.find(id);
    if (bLogin)
    {
        if (auto o = new GXClient(id))
        {
            m_objects[id] = new GXClient(id);
            prepareSocket(o);
        }
    }
    else if (itr!=m_objects.end())
    {
        delete itr->second;
        m_objects.erase(itr);
    }
}

void GXManager::ProcessPostInfo(const Uav2GXMessage &msg)
{
    if (msg.GetSenderType() != ObjectUav::UAVType())
        return;

    auto ms = msg.GetProtobuf();
    ms = ms ? ms->New() : NULL;
    if (ms)
    {
        ms->CopyFrom(*msg.GetProtobuf());
        m_protoSnds.push_back(ms);
    }
}

void GXManager::ProcessEvents()
{
    Lock l(m_mtxM);
    for (auto itr =m_events.begin(); itr!=m_events.end();)
    {
        if (itr->first.empty() && itr->second==GXClient::St_Connect)
        {
            uavLoginGx();
        }
        else
        {
            auto itrO = m_objects.find(itr->first);
            if (itrO != m_objects.end())
            {
                auto o = itrO->second;
                o->SetStat(itr->second);
                if (o->IsChanged())
                {
                    o->ClearChanged();
                    SendMsg(new GX2UavMessage(o->GetID(), o->GetStat()));
                }
            }
        }
        itr = m_events.erase(itr);
    }
    sendPositionInfo();
    checkSocket(Utility::msTimeTick());
}

void GXManager::_prcsLoginAck(AckUavIdentityAuthentication *ack)
{
    if (!ack)
        return;

    PushEvent(ack->uavid(), ack->result() == 1 ? GXClient::St_Authed : GXClient::St_AuthFail);
}

void GXManager::_prcsInformationAck(AckOperationInformation *)
{
}

void GXManager::prepareSocket(GXClient *o)
{
    if (m_sockets.empty() || (m_sockets.size() < 10 && m_protoSnds.size() > 10))
    {
        if (auto s = new GXClientSocket(this))
        {
            s->ConnectTo(GXHost, GXPort);
            m_sockMgr->AddSocket(s);
            m_sockets.push_back(s);
        }
    }
    else if(o && sendUavLogin(o->GetID()))
    {
        o->SetStat(GXClient::St_Authing);
    }
}

void GXManager::uavLoginGx()
{
    for (const pair<string, GXClient*> &itr : m_objects)
    {
        if (itr.second->GetStat() != GXClient::St_Unknow)
            continue;

        if (sendUavLogin(itr.second->GetID()))
            itr.second->SetStat(GXClient::St_Authing);
        else
            break;
    }
}

void GXManager::sendPositionInfo()
{
    auto itr = m_protoSnds.begin();
    if (itr == m_protoSnds.end())
        return;

    for (auto sock : m_sockets)
    {
        if (sock->IsConnect())
        {
            while (itr != m_protoSnds.end())
            {
                if (ObjectAbsPB::SendProtoBuffTo(sock, **itr))
                    itr = m_protoSnds.erase(itr);
                else
                    break;

                if (itr == m_protoSnds.end())
                    return;
            }
        }
    }
}

bool GXManager::sendUavLogin(const std::string &id)
{
    RequestUavIdentityAuthentication req;
    req.set_seqno(1);
    req.set_uavid(id);
    for (auto sock : m_sockets)
    {
        if (sock->IsConnect())
        {
            if (ObjectAbsPB::SendProtoBuffTo(sock, req))
                return true;
        }
    }
    return false;
}

void GXManager::checkSocket(uint64_t ms)
{
    if (ms - m_tmCheck < 1000)
        return;

    m_tmCheck = ms;
    for (auto sock : m_sockets)
    {
        if (!sock->IsConnect())
            sock->Reconnect();
    }
}

DECLARE_MANAGER_ITEM(GXManager)