
/* include ------------------------------------------------------------------ */
#include "eos_test.h"
#include "eventos.h"
#include "event_def.h"
#include "unity.h"
#include <stdbool.h>

// **eos** ---------------------------------------------------------------------
enum {
    EosRun_OK                               = 0,
    EosRun_NotEnabled,
    EosRun_NoEvent,
    EosRun_NoActor,
    EosRun_NoActorSub,

    // Timer
    EosTimer_Empty,
    EosTimer_NotTimeout,
    EosTimer_ChangeToEmpty,

    EosRunErr_NotInitEnd                    = -1,
    EosRunErr_ActorNotSub                   = -2,
    EosRunErr_MallocFail                    = -3,
    EosRunErr_SubTableNull                  = -4,
    EosRunErr_InvalidEventData              = -5,
    EosRunErr_HeapMemoryNotEnough           = -6,
    EosRunErr_TimerRepeated                 = -7,
};

#if (EOS_USE_TIME_EVENT != 0)
#define EOS_MS_NUM_30DAY                    (2592000000)

enum {
    EosTimerUnit_Ms                         = 0,    // 60S, ms
    EosTimerUnit_100Ms,                             // 100Min, 50ms
    EosTimerUnit_Sec,                               // 16h, 500ms
    EosTimerUnit_Minute,                            // 15day, 30S

    EosTimerUnit_Max
};

static const eos_u32_t timer_threshold[EosTimerUnit_Max] = {
    60000,                                          // 60 S
    6000000,                                        // 100 Minutes
    57600000,                                       // 16 hours
    1296000000,                                     // 15 days
};

static const eos_u32_t timer_unit[EosTimerUnit_Max] = {
    1, 100, 1000, 60000
};

typedef struct eos_event_timer {
    eos_u32_t topic                         : 13;
    eos_u32_t oneshoot                      : 1;
    eos_u32_t unit                       : 2;
    eos_u32_t period                        : 16;
    eos_u32_t timeout_ms;
} eos_event_timer_t;
#endif

typedef struct eos_block {
    // word[0]
    eos_u32_t next                          : 15;
    eos_u32_t q_next                        : 15;
    eos_u32_t offset                        : 2;
    // word[1]
    eos_u32_t last                          : 15;
    eos_u32_t q_last                        : 15;
    eos_u32_t free                          : 1;
    // word[2]
    eos_u16_t size                          : 15;
} eos_block_t;

typedef struct eos_event_inner {
    eos_sub_t sub;
    eos_topic_t topic;
} eos_event_inner_t;

typedef struct eos_heap {
    eos_u8_t data[EOS_SIZE_HEAP];
    // word[0]
    eos_u32_t size                          : 15;       /* total size */
    eos_u32_t queue                         : 15;
    eos_u32_t error_id                      : 2;
    // word[2]
    eos_u32_t current                       : 15;
    eos_u32_t empty                         : 1;
    // word[2]
    eos_sub_t sub_general;
    eos_sub_t count;
} eos_heap_t;

typedef struct eos_tag {
#if (EOS_USE_PUB_SUB != 0)
    eos_mcu_t *sub_table;                                     // event sub table
#endif

    eos_mcu_t actor_exist;
    eos_mcu_t actor_enabled;
    eos_actor_t * actor[EOS_MAX_ACTORS];

#if (EOS_USE_EVENT_DATA != 0)
    eos_heap_t heap;
#endif

#if (EOS_USE_TIME_EVENT != 0)
    eos_event_timer_t etimer[EOS_MAX_TIME_EVENT];
    eos_u32_t time;
    eos_u32_t timeout_min;
    eos_u8_t timer_count;
#endif

    eos_u8_t enabled                        : 1;
    eos_u8_t running                        : 1;
    eos_u8_t init_end                       : 1;
} eos_t;
// **eos end** -----------------------------------------------------------------

/* unittest ----------------------------------------------------------------- */
extern int eos_once(void);
extern void * eos_get_framework(void);
extern int eos_evttimer(void);

#if (EOS_USE_PUB_SUB != 0)
static eos_mcu_t eos_sub_table[Event_Max];
#endif

void eos_test_fsm(void)
{

}


