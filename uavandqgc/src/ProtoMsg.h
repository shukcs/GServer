#ifndef  __PROTO_MSG_H__
#define __PROTO_MSG_H__

#include <string>

namespace google {
    namespace protobuf {
        class Message;
    }
}

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif
class ISocket;
#ifdef SOCKETS_NAMESPACE
}
#endif

#define d_p_ClassName(_cls) _cls::descriptor()->full_name()
class ProtoMsg
{
public:
    ProtoMsg();
    ~ProtoMsg();
    const std::string &GetMsgName()const;
    google::protobuf::Message *GetProtoMessage()const;
    google::protobuf::Message *DeatachProto();
    bool Parse(const char *buff, int &len);

    bool SendProto(const google::protobuf::Message &msg, ISocket *s);
    int UnsendLength()const;
    int CopySend(char *buff, int len, unsigned from)const;
    void SetSended(int n);
    void InitSize(uint16_t sz = 2048);
public:
    static bool SendProtoTo(const google::protobuf::Message &msg, ISocket *s);
protected:
    void _parse(const std::string &name, const char *buff, int len);
    bool _resureSz(uint16_t sz);
    void _clear();
    void _reset();
private:
    google::protobuf::Message   *m_msg;
    char                        *m_buff;
    int                         m_size;
    int                         m_len;
    int                         m_sended;
    std::string                 m_name;
};

#endif//__PROTO_MSG_H__

