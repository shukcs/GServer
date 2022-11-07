#include "ObjectAbsPB.h"
#include "socketBase.h"
#include "das.pb.h"
#include "ProtoMsg.h"
#include "Messages.h"
#include "Utility.h"

using namespace das::proto;
using namespace google::protobuf;
using namespace std;

////////////////////////////////////////////////////////////////////////////////
//ObjectUav
////////////////////////////////////////////////////////////////////////////////
ObjectAbsPB::ObjectAbsPB(const std::string &id): IObject(id)
, ILink()
{
}

ObjectAbsPB::~ObjectAbsPB()
{
    delete m_p;
}

bool ObjectAbsPB::SendProtoBuffTo(ISocket *s, const Message &msg)
{
    if (!s)
        return false;

    char buf[256] = { 0 };
    int sz = ProtobufParse::serialize(&msg, buf, 256);
    if (sz > 0)
        return sz==s->Send(sz, buf);

    return false;
}

void ObjectAbsPB::WaitSend(google::protobuf::Message *msg)
{
    auto mgr = GetManager();
    if (!mgr)
    {
        delete msg;
        return;
    }
    auto ret = mgr->SendData2Link(GetObjectID(), msg);

    if (!ret)
        delete msg;
}

IObject *ObjectAbsPB::GetParObject()
{
    return this;
}

ILink *ObjectAbsPB::GetLink()
{
    return this;
}

void ObjectAbsPB::CopyAndSend(const google::protobuf::Message &msg)
{
    if (!IsConnect())
        return;

    if (Message *ms = msg.New())
    {
        ms->CopyFrom(msg);
        WaitSend(ms);
    }
}
////////////////////////////////////////////////////////////////////////////////
//AbsPBManager
////////////////////////////////////////////////////////////////////////////////
AbsPBManager::AbsPBManager()
{
    InitPackPrcs((SerierlizePackFuc)&ProtobufParse::serialize
        , (ParsePackFuc)&ProtobufParse::Parse
        , (RelaesePackFuc)&ProtobufParse::releaseProtobuf);
}

AbsPBManager::~AbsPBManager()
{
}

DeclareRcvPB(PostHeartBeat);
DeclareRcvPB(RequestIdentityAllocation);
DeclareRcvPB(RequestGVIdentityAuthentication);
DeclareRcvPB(RequestIVIdentityAuthentication);
DeclareRcvPB(RequestTrackerIdentityAuthentication);
DeclareRcvPB(AckTrackerIdentityAuthentication);
DeclareRcvPB(Request3rdIdentityAuthentication);
DeclareRcvPB(AckUavIdentityAuthentication);
DeclareRcvPB(UpdateDeviceList);
DeclareRcvPB(AckUpdateDeviceList);
DeclareRcvPB(SyncDeviceList);
DeclareRcvPB(RequestPositionAuthentication);
DeclareRcvPB(PostOperationInformation);
DeclareRcvPB(AckOperationInformation);
DeclareRcvPB(QueryParameters);
DeclareRcvPB(AckQueryParameters);
DeclareRcvPB(ConfigureParameters);
DeclareRcvPB(AckConfigurParameters);
DeclareRcvPB(RequestProgramUpgrade);