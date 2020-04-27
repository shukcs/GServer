#include "ProtoMsg.h"
#include "das.pb.h"
#include "Utility.h"
#include "socketBase.h"
#include <string.h>

#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

enum MyEnum
{
    Max_PBSize = 0x4000,
};

using namespace das::proto;
using namespace std;
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

bool ProtoMsg::Parse(const char *buff, int &len)
{
    int pos = 0;
    int n = 0;
    _clear();
    while (pos+17<len && (n = Utility::FindString(buff+pos, len-pos, PROTOFLAG)) >= 0)
    {
        n -= 8;
        if (n >= 0)
        {
            int szMsg = Utility::fromBigendian(buff + pos);
            pos += n;
            if (szMsg > Max_PBSize)
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

                size_t nameLen = Utility::fromBigendian(buff + pos +4);
                m_name = string(buff + pos + 8, nameLen-1);
                pos += szMsg + 4;
                if (_parse(m_name, buff+pos-szMsg+4+nameLen, szMsg-8-nameLen))
                    break;

                m_name.clear();
                continue;
            }
            break;
        }
        else
        {
            pos += n + 18;
        }
    }
 
    if (n < 0)
        len = len > 17 ? len-17 : 0;
    else if (pos > 0 || n >= 0)
        len = pos;

    return !m_name.empty();
}

bool ProtoMsg::_parse(const std::string &name, const char *buff, int len)
{
    if (!buff || len < 0)
        return false;

    if (name == d_p_ClassName(PostHeartBeat))
        m_msg = new PostHeartBeat;                          //心跳
    else if (name == d_p_ClassName(RequestNewGS))
        m_msg = new RequestNewGS;                           //申请GS用户
    else if (name == d_p_ClassName(RequestIdentityAllocation))
        m_msg = new RequestIdentityAllocation;
    else if (name == d_p_ClassName(RequestGSIdentityAuthentication))
        m_msg = new RequestGSIdentityAuthentication;        //GS登陆
    else if (name == d_p_ClassName(RequestUavIdentityAuthentication))
        m_msg = new RequestUavIdentityAuthentication;       //UAV登陆
    else if (name == d_p_ClassName(PostProgram))
        m_msg = new PostProgram;                            //上传FW
    else if (name == d_p_ClassName(AckNotifyProgram))
        m_msg = new AckNotifyProgram;                       //通知Uav固件升级回复
    else if (name == d_p_ClassName(RequestProgram))
        m_msg = new RequestProgram;                         //Uav FW下载
    else if (name == d_p_ClassName(PostParcelDescription))
        m_msg = new PostParcelDescription;                  //上传地块
    else if (name == d_p_ClassName(DeleteParcelDescription))
        m_msg = new DeleteParcelDescription;                //删除地块
    else if (name == d_p_ClassName(RequestParcelDescriptions))
        m_msg = new RequestParcelDescriptions;              //查询地块
    else if (name == d_p_ClassName(PostOperationDescription))
        m_msg = new PostOperationDescription;               //上传规划
    else if (name == d_p_ClassName(DeleteOperationDescription))
        m_msg = new DeleteOperationDescription;             //删除规划
    else if (name == d_p_ClassName(RequestOperationDescriptions))
        m_msg = new RequestOperationDescriptions;           //查询规划
    else if (name == d_p_ClassName(PostOperationRoute))
        m_msg = new PostOperationRoute;                     //飞机作业
    else if (name == d_p_ClassName(PostOperationInformation))
        m_msg = new PostOperationInformation;               //GPS 位置信息
    else if (name == d_p_ClassName(PostOperationRoute))
        m_msg = new PostOperationRoute;                     //上传航线结果
    else if (name == d_p_ClassName(AckPostOperationRoute))
        m_msg = new AckPostOperationRoute;                 //查询航线结果
    else if (name == d_p_ClassName(SyscOperationRoutes))
        m_msg = new SyscOperationRoutes;                   //同步航线结果
    else if (name == d_p_ClassName(RequestRouteMissions))
        m_msg = new RequestRouteMissions;                  //飞机下载航线
    else if (name == d_p_ClassName(RequestUavMission))
        m_msg = new RequestUavMission;                     //飞机作业查询
    else if (name == d_p_ClassName(RequestUavStatus))
        m_msg = new RequestUavStatus;                      //查询飞机状态结果
    else if (name == d_p_ClassName(RequestBindUav))
        m_msg = new RequestBindUav;                        //请求绑定、解除绑定应答
    else if (name == d_p_ClassName(RequestUavProductInfos))
        m_msg = new RequestUavProductInfos;                //查询飞机信息结果
    else if (name == d_p_ClassName(PostControl2Uav))
        m_msg = new PostControl2Uav;
    else if (name == d_p_ClassName(PostStatus2GroundStation))
        m_msg = new PostStatus2GroundStation;
    else if (name == d_p_ClassName(RequestPositionAuthentication))
        m_msg = new RequestPositionAuthentication;          //查询是否禁飞位置
    else if (name == d_p_ClassName(RequestFriends))
        m_msg = new RequestFriends;
    else if (name == d_p_ClassName(GroundStationsMessage))
        m_msg = new GroundStationsMessage;                  //好友消息
    else if (name == d_p_ClassName(AckGroundStationsMessage))
        m_msg = new AckGroundStationsMessage;               //好友消息服务器回执

    if (m_msg)
        return m_msg->ParseFromArray(buff, len);

    return false;
}

void ProtoMsg::_clear()
{
    ReleasePointer(m_msg);
    m_name.clear();
}
