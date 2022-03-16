
#ifndef EVENT_DEF_H__
#define EVENT_DEF_H__

#include "eventos.h"

enum {
    Event_Test = Event_User,
    Event_TestFsm,
    Event_TestHsm,
    Event_TestReactor,
    Event_Time_500ms,
    Event_Time_2000ms,

    Event_Data,

    Event_ActEnd,
    
    Event_Max
};

#endif
