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

#define DeclareRcvPB(msg) static PBFactoryItem<msg> g_pb##msg;

class ProtoMsg
{
public:
    ProtoMsg();
    ~ProtoMsg();
    const std::string &GetMsgName()const;
    google::protobuf::Message *GetProtoMessage()const;
    google::protobuf::Message *DeatachProto(bool clear=true);
    bool Parse(const char *buff, uint32_t &len);
protected:
    bool _parse(const std::string &name, const char *buff, int len);
    void _clear();
private:
    google::protobuf::Message   *m_msg;
    std::string                 m_name;
};

#endif//__PROTO_MSG_H__

