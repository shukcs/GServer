#include "ProtoMsg.h"
#include "das.pb.h"
#include "Utility.h"
#include "socketBase.h"
#include <string.h>

#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

using namespace das::proto;
#define PROTOFLAG "das.proto."
using namespace std;
////////////////////////////////////////////////////////////////////////////////
//ProtoMsg
////////////////////////////////////////////////////////////////////////////////
ProtoMsg::ProtoMsg():m_msg(NULL), m_buff(NULL), m_size(0), m_len(0), m_sended(0)
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
    int n = Utility::FindString(buff, len, PROTOFLAG);

    _clear();
    while (n >= 0)
    {
        const char *src = buff+pos;
        if (n >= 8)
        {
            n -= 8;
            int szMsg = Utility::fromBigendian(src +n);
            if (szMsg+4 <= len-n)
            {
                uint32_t crc = Utility::fromBigendian(src+n+szMsg);
                if (crc != Utility::Crc32(src+n+4, szMsg-4))
                {
                    pos += n+18;
                    n = Utility::FindString(buff+pos, len, PROTOFLAG);
                    continue;
                }

                size_t nameLen = Utility::fromBigendian(src+n+4);
                int tmp = n + 8 + nameLen;
                m_name = src + n + 8;
                _parse(m_name, src+tmp, szMsg-8-nameLen);
                len = n+szMsg+4;
                return true;
            }
            len = n;
            break;
        }
        pos = n + 18;
        n = Utility::FindString(buff+pos, len-pos, PROTOFLAG);
    }
    pos = len > 17 ? 17 : len;
    len = len - pos;
    return false;
}

bool ProtoMsg::SendProto(const google::protobuf::Message &msg, ISocket *s)
{
    if (RemaimedLength() <= 0)
        _reset();

    string name = msg.GetTypeName();
    if (!s || name.length() < 1)
        return false;

    int nameLen = name.length() + 1;
    int proroLen = msg.ByteSize();
    int len = nameLen + proroLen + 8;
    if (m_len+len+4>0xffff || !_resureSz(m_len+len+4))
        return false;

    char *buf = m_buff+m_len;
    Utility::toBigendian(len, buf);
    Utility::toBigendian(nameLen, buf+4);
    strcpy(buf+8, name.c_str());
    msg.SerializeToArray(buf+nameLen+8, proroLen);
    int crc = Utility::Crc32(buf+4, len-4);
    Utility::toBigendian(crc, buf+len);
    m_len += len+4;
    s->Send(len + 4);

    return true;
}

bool ProtoMsg::SendProtoTo(const google::protobuf::Message &msg, ISocket *s)
{
    string name = msg.GetTypeName();
    if (!s || name.length() < 1)
        return false;

    int nameLen = name.length() + 1;
    int proroLen = msg.ByteSize();
    int len = nameLen + proroLen + 8;
    if (len + 4 > 256)
        return false;

    char buf[256];
    Utility::toBigendian(len, buf);
    Utility::toBigendian(nameLen, buf + 4);
    strcpy(buf + 8, name.c_str());
    msg.SerializeToArray(buf + nameLen + 8, proroLen);
    int crc = Utility::Crc32(buf + 4, len - 4);
    Utility::toBigendian(crc, buf + len);
    s->Send(len + 4, buf);
    return true;
}

int ProtoMsg::RemaimedLength() const
{
    if (m_buff && m_len > 0 && m_sended >= 0 && m_len > m_sended)
        return m_len - m_sended;

    return 0;
}

int ProtoMsg::CopySend(char *buff, int len, unsigned from) const
{
    int nRemain = m_len - m_sended - from;
    if (len > 0 && m_buff && m_len > 0 && m_sended >= 0 && nRemain>0)
    {
        if (len > nRemain)
            len = nRemain;

        memcpy(buff, m_buff + m_sended + from, len);
        return len;
    }
    return 0;
}

void ProtoMsg::SetSended(int n)
{
    if (n < 0)
        _reset();
    else
        m_sended += n;
}

void ProtoMsg::InitSize(uint16_t sz/*=2048*/)
{
    _resureSz(sz);
}

void ProtoMsg::_parse(const std::string &name, const char *buff, int len)
{
    delete DeatachProto(false);

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

    if (m_msg)
        m_msg->ParseFromArray(buff, len);
}

bool ProtoMsg::_resureSz(uint16_t sz)
{
    if (sz > m_size)
    {
        delete m_buff;
        m_buff = new char[sz];
        m_size = m_buff ? sz : 0;
    }

    return m_buff != NULL;
}

void ProtoMsg::_clear()
{
    if (m_msg)
    {
        delete m_msg;
        m_msg = NULL;
    }
    m_name.clear();
}

void ProtoMsg::_reset()
{
    m_sended = 0;
    m_len = 0;
}
