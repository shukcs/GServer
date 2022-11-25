#include "ObjectAbsPB.h"
#include "socketBase.h"
#include "das.pb.h"
#include "ProtoMsg.h"
#include "Messages.h"
#include "Utility.h"
#include "VGMysql.h"
#include "DBExecItem.h"
#include "DBMessages.h"
#include "Lock.h"

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

bool ObjectAbsPB::WaitSend(Message *msg)
{
    auto ret = SendData2Link(msg);
    if (!ret)
        delete msg;

    return ret;
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
    if (!IsConnect())
        return;

    if (Message *ms = msg.New())
    {
        ms->CopyFrom(msg);
        WaitSend(ms);
    }
}