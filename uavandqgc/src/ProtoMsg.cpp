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
        m_msg = new PostHeartBeat;                          //����
    else if (name == d_p_ClassName(RequestNewGS))
        m_msg = new RequestNewGS;                           //����GS�û�
    else if (name == d_p_ClassName(RequestIdentityAllocation))
        m_msg = new RequestIdentityAllocation;
    else if (name == d_p_ClassName(RequestGSIdentityAuthentication))
        m_msg = new RequestGSIdentityAuthentication;        //GS��½
    else if (name == d_p_ClassName(RequestUavIdentityAuthentication))
        m_msg = new RequestUavIdentityAuthentication;       //UAV��½
    else if (name == d_p_ClassName(PostProgram))
        m_msg = new PostProgram;                            //�ϴ�FW
    else if (name == d_p_ClassName(AckNotifyProgram))
        m_msg = new AckNotifyProgram;                       //֪ͨUav�̼������ظ�
    else if (name == d_p_ClassName(RequestProgram))
        m_msg = new RequestProgram;                         //Uav FW����
    else if (name == d_p_ClassName(PostParcelDescription))
        m_msg = new PostParcelDescription;                  //�ϴ��ؿ�
    else if (name == d_p_ClassName(DeleteParcelDescription))
        m_msg = new DeleteParcelDescription;                //ɾ���ؿ�
    else if (name == d_p_ClassName(RequestParcelDescriptions))
        m_msg = new RequestParcelDescriptions;              //��ѯ�ؿ�
    else if (name == d_p_ClassName(PostOperationDescription))
        m_msg = new PostOperationDescription;               //�ϴ��滮
    else if (name == d_p_ClassName(DeleteOperationDescription))
        m_msg = new DeleteOperationDescription;             //ɾ���滮
    else if (name == d_p_ClassName(RequestOperationDescriptions))
        m_msg = new RequestOperationDescriptions;           //��ѯ�滮
    else if (name == d_p_ClassName(PostOperationRoute))
        m_msg = new PostOperationRoute;                     //�ɻ���ҵ
    else if (name == d_p_ClassName(PostOperationInformation))
        m_msg = new PostOperationInformation;               //GPS λ����Ϣ
    else if (name == d_p_ClassName(PostOperationRoute))
        m_msg = new PostOperationRoute;                     //�ϴ����߽��
    else if (name == d_p_ClassName(AckPostOperationRoute))
        m_msg = new AckPostOperationRoute;                 //��ѯ���߽��
    else if (name == d_p_ClassName(SyscOperationRoutes))
        m_msg = new SyscOperationRoutes;                   //ͬ�����߽��
    else if (name == d_p_ClassName(RequestRouteMissions))
        m_msg = new RequestRouteMissions;                  //�ɻ����غ���
    else if (name == d_p_ClassName(RequestUavMission))
        m_msg = new RequestUavMission;                     //�ɻ���ҵ��ѯ
    else if (name == d_p_ClassName(RequestUavStatus))
        m_msg = new RequestUavStatus;                      //��ѯ�ɻ�״̬���
    else if (name == d_p_ClassName(RequestBindUav))
        m_msg = new RequestBindUav;                        //����󶨡������Ӧ��
    else if (name == d_p_ClassName(RequestUavProductInfos))
        m_msg = new RequestUavProductInfos;                //��ѯ�ɻ���Ϣ���
    else if (name == d_p_ClassName(PostControl2Uav))
        m_msg = new PostControl2Uav;
    else if (name == d_p_ClassName(PostStatus2GroundStation))
        m_msg = new PostStatus2GroundStation;
    else if (name == d_p_ClassName(RequestPositionAuthentication))
        m_msg = new RequestPositionAuthentication;          //��ѯ�Ƿ����λ��
    else if (name == d_p_ClassName(RequestFriends))
        m_msg = new RequestFriends;
    else if (name == d_p_ClassName(GroundStationsMessage))
        m_msg = new GroundStationsMessage;                  //������Ϣ
    else if (name == d_p_ClassName(AckGroundStationsMessage))
        m_msg = new AckGroundStationsMessage;               //������Ϣ��������ִ
    else if (name == d_p_ClassName(PostMissionSuspend))
        m_msg = new PostMissionSuspend;               //������Ϣ��������ִ
    else if (name == d_p_ClassName(RequestUavMissionAcreage))
        m_msg = new RequestUavMissionAcreage;               //������Ϣ��������ִ

    if (m_msg)
        return m_msg->ParseFromArray(buff, len);

    return false;
}

void ProtoMsg::_clear()
{
    ReleasePointer(m_msg);
    m_name.clear();
}
