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

#define PROTOFLAG "das.proto."
#define d_p_ClassName(_cls) _cls::descriptor()->full_name()
class ProtoMsg
{
public:
    ProtoMsg();
    ~ProtoMsg();
    const std::string &GetMsgName()const;
    google::protobuf::Message *GetProtoMessage()const;
    google::protobuf::Message *DeatachProto(bool clear=true);
    bool Parse(const char *buff, int &len);
protected:
    bool _parse(const std::string &name, const char *buff, int len);
    void _clear();
private:
    google::protobuf::Message   *m_msg;
    std::string                 m_name;
};

#endif//__PROTO_MSG_H__

