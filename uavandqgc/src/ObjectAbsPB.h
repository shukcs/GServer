#ifndef  __OBJECT_ABS_PB_H__
#define __OBJECT_ABS_PB_H__

#include "ObjectBase.h"

namespace google {
    namespace protobuf {
        class Message;
    }
}

class ExecutItem;
class VGMySql;
#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif

class ProtoMsg;
class ObjectAbsPB : public IObject
{
public:
    ObjectAbsPB(const std::string &id);
    ~ObjectAbsPB();

    bool IsConnect()const;
protected:
    void OnConnected(bool bConnected);
    int GetSenLength()const;
    int CopySend(char *buf, int sz, unsigned form = 0);
    void SetSended(int sended = -1);    //-1,发送完

    //向下实现
    virtual VGMySql *GetMySql()const;
 
    void send(const google::protobuf::Message &msg);
    int64_t executeInsertSql(ExecutItem *item);
protected:
    bool            m_bConnect;
    ProtoMsg        *m_p;
};

#endif // __OBJECT_ABS_PB_H__

