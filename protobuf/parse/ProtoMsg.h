#ifndef  __PROTO_MSG_H__
#define __PROTO_MSG_H__

#include <string>
#include <map>

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

class PBAbSFactoryItem
{
public:
    PBAbSFactoryItem(const std::string &name);
    virtual~PBAbSFactoryItem() {}
    virtual google::protobuf::Message *Create()const = 0;
public:
    static google::protobuf::Message *createMessage(const std::string &name);
};

template<class C>
class PBFactoryItem : public PBAbSFactoryItem
{
public:
    PBFactoryItem() : PBAbSFactoryItem(C::descriptor()->full_name()) {}
protected:
    google::protobuf::Message *Create()const
    {
        return new C;
    }
};

namespace ProtobufParse {
    google::protobuf::Message *Parse(const char *buff, uint32_t &len);
    uint32_t serialize(const google::protobuf::Message *msg, char*buf, uint32_t sz);
    void releaseProtobuf(google::protobuf::Message *msg);
};

#define DeclareRcvPB(msg) static PBFactoryItem<msg> g_pb##msg;
#endif//__PROTO_MSG_H__

