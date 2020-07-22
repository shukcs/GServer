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
            if (szMsg > Max_PBSize || szMsg<8)
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

    if (name == d_p_ClassName(PostHeartBeat))
        m_msg = new PostHeartBeat;                                          //申请GS用户
    else if (name == d_p_ClassName(RequestIdentityAllocation))
        m_msg = new RequestIdentityAllocation;
    else if (name == d_p_ClassName(RequestGVIdentityAuthentication))
        m_msg = new RequestGVIdentityAuthentication;        //GS登陆
    else if (name == d_p_ClassName(RequestIVIdentityAuthentication))
        m_msg = new RequestIVIdentityAuthentication;        //GS登陆
    else if (name == d_p_ClassName(RequestTrackerIdentityAuthentication))
        m_msg = new RequestTrackerIdentityAuthentication;    //Tracker登陆
    else if (name == d_p_ClassName(AckTrackerIdentityAuthentication))
        m_msg = new AckTrackerIdentityAuthentication;    //Tracker登陆
    else if (name == d_p_ClassName(Request3rdIdentityAuthentication))
        m_msg = new Request3rdIdentityAuthentication;    //一飞飞机登陆
    else if (name == d_p_ClassName(AckUavIdentityAuthentication))
        m_msg = new AckUavIdentityAuthentication;       //一飞飞机国欣登陆ack
    else if (name == d_p_ClassName(UpdateDeviceList))
        m_msg = new UpdateDeviceList;                       //设备更新
    else if (name == d_p_ClassName(AckUpdateDeviceList))
        m_msg = new AckUpdateDeviceList;                    //设备更新
    else if (name == d_p_ClassName(SyncDeviceList))
        m_msg = new SyncDeviceList;                         //设备查询
    else if (name == d_p_ClassName(RequestPositionAuthentication))
        m_msg = new RequestPositionAuthentication;          //查询是否禁飞位置
    else if (name == d_p_ClassName(PostOperationInformation))
        m_msg = new PostOperationInformation;                //设备位置
    else if (name == d_p_ClassName(AckOperationInformation))
        m_msg = new AckOperationInformation;                //设备位置
    else if (name == d_p_ClassName(QueryParameters))
        m_msg = new QueryParameters;                        //参数查询
    else if (name == d_p_ClassName(AckQueryParameters))
        m_msg = new AckQueryParameters;                     //参数回应
    else if (name == d_p_ClassName(ConfigureParameters))
        m_msg = new ConfigureParameters;                        //参数修改
    else if (name == d_p_ClassName(AckConfigurParameters))
        m_msg = new AckConfigurParameters;                  //参数修改结果
    else if (name == d_p_ClassName(RequestProgramUpgrade))
        m_msg = new RequestProgramUpgrade;                  //参数修改结果

    if (m_msg)
        return m_msg->ParseFromArray(buff, len);

    return false;
}

void ProtoMsg::_clear()
{
    ReleasePointer(m_msg);
    m_name.clear();
}
