
/* include ------------------------------------------------------------------ */
#include "eos_test.h"
#include "event_def.h"
#include "unity.h"
#include "unity_pack.h"

// **eos** ---------------------------------------------------------------------
enum {
    EosRun_OK                           = 0,
    EosRun_NotEnabled,
    EosRun_NoEvent,
    EosRun_NoActor,
    EosRun_NoActorSub,

    // Timer
    EosTimer_Empty,
    EosTimer_NotTimeout,
    EosTimer_ChangeToEmpty,

    EosRunErr_NotInitEnd                = -1,
    EosRunErr_ActorNotSub               = -2,
    EosRunErr_MallocFail                = -3,
    EosRunErr_SubTableNull              = -4,
    EosRunErr_InvalidEventData          = -5,
    EosRunErr_HeapMemoryNotEnough       = -6,
};

#if (EOS_USE_TIME_EVENT != 0)
typedef struct eos_event_timer {
    eos_topic_t topic;
    eos_bool_t is_one_shoot;
    eos_s32_t time_ms_delay;
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
    // word[1]
    eos_sub_t sub_general;
} eos_heap_t;

typedef struct eos_tag {
    eos_mcu_t magic;
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
    eos_event_timer_t e_timer_pool[EOS_MAX_TIME_EVENT];
    eos_u32_t flag_etimerpool[EOS_MAX_TIME_EVENT / 32 + 1];    // timer pool flag
    eos_u32_t timeout_ms_min;
    eos_u32_t time_crt_ms;
    eos_bool_t etimerpool_empty;
#endif

    eos_bool_t enabled;
    eos_bool_t running;
    eos_bool_t init_end;
} eos_t;
// **eos end** -----------------------------------------------------------------


/* unit test ---------------------------------------------------------------- */
static eos_mcu_t sub_table[Event_Max];
static fsm_t fsm;

/* eventos API for test ----------------------------------------------------- */
eos_s8_t eos_once(void);
eos_s8_t eos_event_pub_ret(eos_topic_t topic, void *data, eos_u32_t size);

void eos_test_event(void)
{
    eos_s8_t ret;

    TEST_ASSERT_EQUAL_INT8(EosRunErr_NotInitEnd, eos_once());

    // 测试订阅表的初始化 --------------------------------------------------------
    eventos_init();

#if (EOS_USE_PUB_SUB != 0)
    // 检查尚未设置订阅表
    TEST_ASSERT_EQUAL_INT8(EosRunErr_SubTableNull, eos_once());
    TEST_ASSERT_EQUAL_INT8(EosRunErr_SubTableNull, eos_event_pub_ret(Event_Test, EOS_NULL, 0));
    eos_sub_init(sub_table, Event_Max);
#endif

    // 测试框架的停止
    eventos_stop();
    TEST_ASSERT_EQUAL_INT8(EosRun_NotEnabled, eos_once());

    // 测试尚未注册Actor的情况
    eventos_init();
    eos_sub_init(sub_table, Event_Max);
    TEST_ASSERT_EQUAL_INT8(EosRun_NoActor, eos_event_pub_ret(Event_Test, EOS_NULL, 0));
    TEST_ASSERT_EQUAL_INT8(EosRun_NoActor, eos_once());

    // 注册Actor
    fsm_init(&fsm, 0, EOS_NULL);
    TEST_ASSERT_EQUAL_INT8(EosRun_NoEvent, eos_once());
    TEST_ASSERT_EQUAL_INT8(EosRun_NoActorSub, eos_event_pub_ret(Event_Test, EOS_NULL, 0));

    // eos_event_pub_ret


}
