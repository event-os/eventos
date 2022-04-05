#ifndef EOS_TEST_DEF_H_
#define EOS_TEST_DEF_H_

#include "eventos.h"

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

    EosRet_Max,

    EosRunErr_NotInitEnd                    = -1,
    EosRunErr_ActorNotSub                   = -2,
    EosRunErr_MallocFail                    = -3,
    EosRunErr_SubTableNull                  = -4,
    EosRunErr_InvalidEventData              = -5,
    EosRunErr_HeapMemoryNotEnough           = -6,
    EosRunErr_TimerRepeated                 = -7,
};

typedef uint32_t (* hash_algorithm_t)(const char *string);

#if (EOS_USE_TIME_EVENT != 0)
#define EOS_MS_NUM_30DAY                    (2592000000)

enum {
    EosTimerUnit_Ms                         = 0,    // 60S, ms
    EosTimerUnit_100Ms,                             // 100Min, 50ms
    EosTimerUnit_Sec,                               // 16h, 500ms
    EosTimerUnit_Minute,                            // 15day, 30S

    EosTimerUnit_Max
};

static const uint32_t timer_threshold[EosTimerUnit_Max] = {
    60000,                                          // 60 S
    6000000,                                        // 100 Minutes
    57600000,                                       // 16 hours
    1296000000,                                     // 15 days
};

static const uint32_t timer_unit[EosTimerUnit_Max] = {
    1, 100, 1000, 60000
};

typedef struct eos_event_timer {
    const char *topic;
    uint32_t oneshoot                      : 1;
    uint32_t unit                          : 2;
    uint32_t period                        : 16;
    uint32_t timeout_ms;
} eos_event_timer_t;
#endif

typedef struct eos_block {
    // word[0]
    uint32_t next                          : 15;
    uint32_t q_next                        : 15;
    // word[1]
    uint32_t last                          : 15;
    uint32_t q_last                        : 15;
    uint32_t free                          : 1;
    // word[2]
    uint16_t size                          : 15;
    uint32_t offset                        : 8;
} eos_block_t;

typedef struct eos_event_inner {
    uint32_t sub;
    const char *topic;
} eos_event_inner_t;

typedef struct eos_heap {
    uint8_t data[EOS_SIZE_HEAP];
    // word[0]
    uint32_t size                          : 15;       /* total size */
    uint32_t queue                         : 15;
    uint32_t error_id                      : 2;
    // word[2]
    uint32_t current                       : 15;
    uint32_t empty                         : 1;
    // word[2]
    uint64_t sub_general;
    uint32_t count;
} eos_heap_t;

typedef union eos_obj_block {
    uint32_t event_sub;
    eos_task_t *task;
    eos_heap_t *heap;
    // eos_timer_t *timer;
    // eos_device_t *device;
    void *other;
} eos_obj_block_t;

typedef struct eos_object {
    const char *key;                                    // Key
    eos_obj_block_t block;                              // object block
    uint32_t type;                                      // Object type
} eos_object_t;

typedef struct eos_hash_table {
    eos_object_t object[EOS_MAX_EVENTS];
    uint32_t prime_max;                                 // prime max
    uint32_t size;
} eos_hash_table_t;

typedef struct eos_tag {
    eos_hash_table_t hash;
    hash_algorithm_t hash_func;
    uint64_t task_exist;
    uint64_t actor_enabled;
    eos_task_t * actor[EOS_MAX_TASKS];

#if (EOS_USE_EVENT_DATA != 0)
    eos_heap_t heap;
#endif

#if (EOS_USE_TIME_EVENT != 0)
    eos_event_timer_t etimer[EOS_MAX_TIME_EVENT];
    uint32_t time;
    uint32_t timeout_min;
    uint8_t timer_count;
#endif
    uint32_t delay;

    uint8_t enabled                        : 1;
    uint8_t running                        : 1;
    uint8_t init_end                       : 1;
} eos_t;

/* eventos API for test ----------------------------- */
int8_t eos_execute(uint8_t priority);
int8_t eos_event_pub_ret(const char *topic, void *data, uint32_t size);
void * eos_get_framework(void);
void eos_event_pub_time(const char *topic, uint32_t time_ms, eos_bool_t oneshoot);
void eos_set_time(uint32_t time_ms);
void eos_set_hash(hash_algorithm_t hash);

// **eos end** -----------------------------------------------------------------

#endif
