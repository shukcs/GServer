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
}

void ObjectAbsPB::WaitSend(google::protobuf::Message *msg)
{
    if (!SendData2Link(msg))
        delete msg;
}

const IObject *ObjectAbsPB::GetParObject()const
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