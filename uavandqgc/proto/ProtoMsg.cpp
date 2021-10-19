#include "ProtoMsg.h"
#include "das.pb.h"
#include "Utility.h"
#include "socketBase.h"
#include <string.h>

#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

using namespace das::proto;
using namespace std;
enum MyEnum
{
    Max_PBSize = 0x4000,
};
////////////////////////////////////////////////////////////////////////////////
//PBAbSFactoryItem
////////////////////////////////////////////////////////////////////////////////
map<string, PBAbSFactoryItem*> s_MapPbCreate;

PBAbSFactoryItem::PBAbSFactoryItem(const string &name)
{
    s_MapPbCreate[name] = this;
}

google::protobuf::Message *PBAbSFactoryItem::createMessage(const string &name)
{
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

    auto itr = s_MapPbCreate.find(name);
    if (itr != s_MapPbCreate.end())
        return itr->second->Create();
    return NULL;
}
////////////////////////////////////////////////////////////////////////////////
//ProtoMsg
////////////////////////////////////////////////////////////////////////////////
ProtoMsg::ProtoMsg():m_msg(NULL)
{
}

ProtoMsg::~ProtoMsg()
{
    delete m_msg;
}

const string &ProtoMsg::GetMsgName() const
{
    return m_name;
}

google::protobuf::Message *ProtoMsg::GetProtoMessage() const
{
    return m_msg;
}

google::protobuf::Message *ProtoMsg::DeatachProto(bool clear)
{
    google::protobuf::Message *ret = m_msg;
    m_msg = NULL;
    if(clear)
        m_name.clear();

    return ret;
}

bool ProtoMsg::Parse(const char *buff, uint32_t &len)
{
    uint32_t pos = 0;
    uint32_t n = 0;
    _clear();
    while (pos+17<len && (n = Utility::FindString(buff+pos, len-pos, PROTOFLAG)) >= 0)
    {
        if (n >= 8)
        {
            pos += n-8;
            uint32_t szMsg = Utility::fromBigendian(buff+pos);
            if (szMsg>Max_PBSize || szMsg<18)
            {
                pos += 18;
                continue;
            }

            if (szMsg+4 <= len-pos)
            {
                uint32_t crc = Utility::Crc32(buff+pos+4, szMsg-4);
                if (crc != (uint32_t)Utility::fromBigendian(buff+pos+szMsg))
                {
                    pos += 18;
                    continue;
                }

                uint32_t nameLen = Utility::fromBigendian(buff + pos + 4);
                m_name = string(buff + pos + 8, nameLen - 1);
                uint32_t tmp = pos + 8 + nameLen;
                pos += szMsg + 4;
                if (_parse(m_name, buff+tmp, szMsg-8-nameLen))
                    break;

                m_name.clear();
                continue;
            }
            break;
        }
        else
        {
            pos += n + 10;
        }
    }
 
    if (n < 8)
        len = len > 17 ? len-17 : 0;
    else if (pos > 0 || n >= 8)
        len = pos;

    return !m_name.empty();
}

bool ProtoMsg::_parse(const std::string &name, const char *buff, int len)
{
    if (!buff || len < 0)
        return false;
  
    m_msg = PBAbSFactoryItem::createMessage(name);
    bool ret = false;
    if (!m_msg)
        return ret;

    try {
        ret = m_msg->ParseFromArray(buff, len);
    }
    catch (...)
    {
        throw;
    }

    return false;
}

void ProtoMsg::_clear()
{
    ReleasePointer(m_msg);
    m_name.clear();
}
