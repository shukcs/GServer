#ifndef  __MESSGES_H__
#define __MESSGES_H__

#include "ObjectBase.h"

namespace google {
    namespace protobuf {
        class Message;
    }
}

enum MessageType
{
    Unknown,
    BindUav,
    ControlUav,
    SychMission,
    QueryUav,

    BindUavRes,
    ControlUavRes,
    SychMissionRes,
    PushUavSndInfo,
    ControlGs,
    QueryUavRes,
};

#endif//__MESSGES_H__

