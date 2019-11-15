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
namespace SOCKETS_NAMESPACE {
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
    void SetSended(int sended = -1);    //-1,������

    //����ʵ��
    virtual VGMySql *GetMySql()const;

    bool send(const google::protobuf::Message &msg);
    int64_t executeInsertSql(ExecutItem *item);
protected:
    bool            m_bConnect;
    ProtoMsg        *m_p;
};

#ifdef SOCKETS_NAMESPACE
}
#endif
#endif // __OBJECT_ABS_PB_H__

