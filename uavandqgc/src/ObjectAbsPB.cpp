#include "ObjectAbsPB.h"
#include "socketBase.h"
#include "das.pb.h"
#include "ProtoMsg.h"
#include "Messages.h"
#include "Utility.h"
#include "VGMysql.h"
#include "DBExecItem.h"
#include "DBMessages.h"

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

void ObjectAbsPB::CopyAndSend(const Message &msg)
{
    if (!IsLinked())
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

void AbsPBManager::ToCurrntLog(int err, const std::string &obj, int evT, const std::string &dscb)
{
    if (auto msg = new DBMessage(this, "DBLog"))
    {
        msg->SetSql("insertLog");

        msg->SetWrite("event", evT);
        msg->SetWrite("error", err);
        msg->SetWrite("object", obj);
        msg->SetWrite("evntdesc", dscb);
        if (!SendMsg(msg))
            delete msg;
    }
}

DeclareRcvPB(PostHeartBeat);
DeclareRcvPB(RequestNewGS);
DeclareRcvPB(RequestIdentityAllocation);
DeclareRcvPB(AckUavIdentityAuthentication);
DeclareRcvPB(RequestGSIdentityAuthentication);
DeclareRcvPB(RequestUavIdentityAuthentication);
DeclareRcvPB(PostProgram);
DeclareRcvPB(AckNotifyProgram);
DeclareRcvPB(RequestProgram);
DeclareRcvPB(SyncDeviceList);
DeclareRcvPB(AckUpdateDeviceList);
DeclareRcvPB(PostParcelDescription);
DeclareRcvPB(DeleteParcelDescription);
DeclareRcvPB(RequestParcelDescriptions);
DeclareRcvPB(PostOperationDescription);
DeclareRcvPB(DeleteOperationDescription);
DeclareRcvPB(RequestOperationDescriptions);
DeclareRcvPB(PostOperationRoute);
DeclareRcvPB(PostOperationInformation);
DeclareRcvPB(AckOperationInformation);
DeclareRcvPB(AckPostOperationRoute);
DeclareRcvPB(SyscOperationRoutes);
DeclareRcvPB(RequestRouteMissions);
DeclareRcvPB(RequestUavStatus);
DeclareRcvPB(RequestBindUav);
DeclareRcvPB(RequestUavProductInfos);
DeclareRcvPB(PostControl2Uav);
DeclareRcvPB(PostStatus2GroundStation);
DeclareRcvPB(RequestPositionAuthentication);
DeclareRcvPB(RequestFriends);
DeclareRcvPB(GroundStationsMessage);
DeclareRcvPB(AckGroundStationsMessage);
DeclareRcvPB(RequestMissionSuspend);
DeclareRcvPB(RequestUavMissionAcreage);
DeclareRcvPB(RequestUavMission);
DeclareRcvPB(PostOperationAssist);
DeclareRcvPB(RequestOperationAssist);
DeclareRcvPB(PostABPoint);
DeclareRcvPB(RequestABPoint);
DeclareRcvPB(PostOperationReturn);
DeclareRcvPB(RequestOperationReturn);
DeclareRcvPB(PostBlocks);
DeclareRcvPB(PostABOperation);