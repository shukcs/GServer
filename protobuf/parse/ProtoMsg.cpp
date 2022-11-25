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
    auto itr = s_MapPbCreate.find(name);
    if (itr != s_MapPbCreate.end())
        return itr->second->Create();
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////
//ProtobufParse
////////////////////////////////////////////////////////////////////////////////
static google::protobuf::Message *parseProtobuf(const std::string &name, const char *buff, int len)
{
    if (!buff || len < 0)
        return NULL;

    auto msg = PBAbSFactoryItem::createMessage(name);
    if (!msg)
        return NULL;

    try {
        if (msg->ParseFromArray(buff, len))
            return msg;
    }
    catch (...)
    {
        delete msg;
        return NULL;
    }
    return NULL;
}

google::protobuf::Message *ProtobufParse::Parse(const char *buff, uint32_t &len)
{
    uint32_t pos = 0;
    while (len > pos + 17)
    {
        int n = Utility::FindString(buff + pos, len - pos, PROTOFLAG);
        if (n < 0)
        {
            pos = len - 17;
            break;
        }   
        if (n < 8)
        {
            pos += n + 10;
            continue;
        }

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
            string name = string(buff + pos + 8, nameLen - 1);
            uint32_t tmp = pos + 8 + nameLen;
            pos += szMsg + 4;
            if (auto msg = parseProtobuf(name, buff + tmp, szMsg - 8 - nameLen))
            {
                len = pos;
                return msg;
            }
        }
    }

    len = pos;
    return NULL;
}

uint32_t ProtobufParse::serialize(const google::protobuf::Message *msg, char*buf, uint32_t sz)
{
    if (!msg || !buf)
        return 0;

    const string &name = msg->GetDescriptor()->full_name();
    if (name.length() < 1)
        return 0;
    uint32_t nameLen = name.length() + 1;
    uint32_t proroLen = msg->ByteSize();
    uint32_t len = nameLen + proroLen + 8;
    if (sz < len + 4)
        return 0;

    Utility::toBigendian(len, buf);
    Utility::toBigendian(nameLen, buf + 4);
    strcpy(buf + 8, name.c_str());
    msg->SerializeToArray(buf + nameLen + 8, proroLen);
    int crc = Utility::Crc32(buf + 4, len - 4);
    Utility::toBigendian(crc, buf + len);
    return len + 4;
}

void ProtobufParse::releaseProtobuf(google::protobuf::Message *msg)
{
    delete msg;
}
