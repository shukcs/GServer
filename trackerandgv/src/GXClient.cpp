#include "GXClient.h"
#include "das.pb.h"
#include "Utility.h"
#include "ObjectManagers.h"
#include "GXLink.h"
#include "ObjectTracker.h"
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
    GXLinkThread(ISocketManager *mgr) :Thread(true), m_mgr(mgr)
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
//ObjectGXClinet
////////////////////////////////////////////////////////////////////////////////
ObjectGXClinet::ObjectGXClinet(const std::string &id) : IObject(id)
, m_gxClient(NULL), m_seq(1), m_bConnect(false), m_mtx(NULL)
, m_stat(St_Unconnect), m_p(new ProtoMsg)
{
    m_buff.ReSize(2048);
}

ObjectGXClinet::~ObjectGXClinet()
{
    delete m_gxClient;
    delete m_p;
}

void ObjectGXClinet::OnRead(ISocket *s, const void *buff, int len)
{
    if (m_gxClient == s && buff && len>0)
    {
        m_buff.Push(buff, len);
        _prcsRcv();
    }
}

void ObjectGXClinet::Login(bool b)
{
    if (b)
    {
        if (!m_gxClient)
        {
            m_gxClient = new GXClientSocket(this);
            if (m_gxClient)
            {
                m_gxClient->ConnectTo(GXHost, GXPort);
                if (auto sg = ((GXClinetManager*)GetManager())->GetSocketManager())
                    sg->AddSocket(m_gxClient);
            }
        }
        else
        {
            m_gxClient->Reconnect();
        }
    }
    else if (m_gxClient)
    {
        m_gxClient->Close();
    }
}

void ObjectGXClinet::OnConnect(bool b)
{
    if (b)
    {
        Lock l(m_mtx);
        RequestTrackerIdentityAuthentication s;
        s.set_seqno(m_seq++);
        s.set_trackerid(GetObjectID());
        ObjectAbsPB::SendProtoBuffTo(m_gxClient, s);
        m_stat = ObjectGXClinet::St_Authing;
    }
    else
    {
        printf("ObjectGXClinet %s disconnect\n", GetObjectID().c_str());
        m_stat = ObjectGXClinet::St_Unconnect;
    }
    m_bConnect = b;
}

void ObjectGXClinet::Send(const google::protobuf::Message &msg)
{
    if (m_gxClient)
    {
        Lock l(m_mtx);
        if (m_bConnect)
            ObjectAbsPB::SendProtoBuffTo(m_gxClient, msg);
        else
            m_gxClient->Reconnect();
    }
    SendMsg(new GX2TrackerMessage(this, m_stat));
}

void ObjectGXClinet::SetMutex(IMutex *mtx)
{
    m_mtx = mtx;
}

int ObjectGXClinet::GetObjectType() const
{
    return GXClinetType();
}

void ObjectGXClinet::InitObject()
{
    if (m_stInit != Uninitial)
        return;

    m_stInit = IObject::Initialed;
}

void ObjectGXClinet::ProcessMessage(IMessage *)
{
    if (m_stInit == Uninitial)
        InitObject();
}

void ObjectGXClinet::_prcsRcv()
{
    auto mgr = (GXClinetManager*)GetManager();
    char *buf = mgr->GetBuff();
    uint32_t rd = m_buff.CopyData(buf, mgr->BuffLength());
    if (m_p)
    {
        int pos = 0;
        uint32_t tmp = rd;
        while (tmp > 18 && m_p->Parse((char*)buf + pos, tmp))
        {
            pos += tmp;
            tmp = rd - pos;
            const string &name = m_p->GetMsgName();
            if (name == d_p_ClassName(AckTrackerIdentityAuthentication))
            {
                m_stat = ((AckTrackerIdentityAuthentication*)m_p->GetProtoMessage())->result() == 1 ? St_Authed : St_AuthFail;
            }
            if (name == d_p_ClassName(AckOperationInformation))
            {
                m_stat = St_AcceptPos;
            }
        }
        pos += tmp;
        rd = pos;
    }
    m_buff.Clear(rd);
}

int ObjectGXClinet::GXClinetType()
{
    return IObject::User + 2;
}
////////////////////////////////////////////////////////////////////////////////
//GXClinetManager
////////////////////////////////////////////////////////////////////////////////
GXClinetManager::GXClinetManager():IObjectManager()
, m_sockMgr(GSocketManager::CreateManager(0,10000))
{
    if (m_sockMgr)
        m_thread = new GXLinkThread(m_sockMgr);
}

GXClinetManager::~GXClinetManager()
{
    delete m_sockMgr;
}

char *GXClinetManager::GetBuff()
{
    return m_bufPublic;
}

int GXClinetManager::BuffLength() const
{
    return 1024;
}

ISocketManager *GXClinetManager::GetSocketManager() const
{
    return m_sockMgr;
}

int GXClinetManager::GetObjectType() const
{
    return ObjectGXClinet::GXClinetType();
}

bool GXClinetManager::PrcsPublicMsg(const IMessage &msg)
{
    switch (msg.GetMessgeType())
    {
    case ObjectEvent::E_Login:
        ProcessLogin(msg.GetSenderID(), msg.GetSenderType(), true); break;
    case IMessage::PushUavSndInfo:
        ProcessPostInfo(*(TrackerMessage*)&msg); break;
    default:
        break;
    }
    return true;
}

IObject *GXClinetManager::PrcsNotObjectReceive(ISocket *, const char *, int)
{
    return NULL;
}

void GXClinetManager::LoadConfig()
{
    InitThread(1, 0);
}

bool GXClinetManager::IsReceiveData() const
{
    return false;
}

void GXClinetManager::ProcessLogin(const std::string sender, int sendTy, bool bLogin)
{
    if (sendTy != ObjectTracker::TrackerType() || sender.empty())
        return;

    auto obj = (ObjectGXClinet*)GetObjectByID(sender);
    if (!obj && bLogin)
    {
        obj = new ObjectGXClinet(sender);
        if (obj)
        { 
            AddObject(obj);
            obj->SetMutex(m_mtx);
        }
    }

    if (obj)
        obj->Login(bLogin);
}

void GXClinetManager::ProcessPostInfo(const TrackerMessage &msg)
{
    if (msg.GetSenderType() != ObjectTracker::TrackerType())
        return;

    if (auto obj = (ObjectGXClinet*)GetObjectByID(msg.GetSenderID()))
        obj->Send(*msg.GetProtobuf());
}


DECLARE_MANAGER_ITEM(GXClinetManager)