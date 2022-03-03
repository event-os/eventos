
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
    EosTimer_Repeated,

    EosRunErr_NotInitEnd                = -1,
    EosRunErr_ActorNotSub               = -2,
    EosRunErr_MallocFail                = -3,
    EosRunErr_SubTableNull              = -4,
    EosRunErr_InvalidEventData          = -5,
    EosRunErr_HeapMemoryNotEnough       = -6,
};

#if (EOS_USE_TIME_EVENT != 0)
typedef struct eos_event_timer {
    eos_u32_t topic                         : 14;
    eos_u32_t oneshoot                      : 1;
    eos_u32_t unit_ms                       : 1;
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
    eos_event_timer_t etimer[EOS_MAX_TIME_EVENT];
    eos_u32_t timeout_min;
    eos_u8_t timer_count;
#endif

    eos_u8_t enabled                    : 1;
    eos_u8_t running                    : 1;
    eos_u8_t init_end                   : 1;
} eos_t;
// **eos end** -----------------------------------------------------------------

#if (EOS_USE_TIME_EVENT != 0)
/* unit test ---------------------------------------------------------------- */
static eos_mcu_t sub_table[Event_Max];
static fsm_t fsm;
static eos_t *f;

/* eventos API for test ----------------------------------------------------- */
eos_s8_t eos_once(void);
eos_s8_t eos_event_pub_ret(eos_topic_t topic, void *data, eos_u32_t size);
void * eos_get_framework(void);
eos_s8_t eos_event_pub_time(eos_topic_t topic, eos_u32_t time_ms, eos_bool_t oneshoot);
#endif

void eos_test_etimer(void)
{
#if (EOS_USE_TIME_EVENT != 0)
    eos_s8_t ret;
    f = eos_get_framework();
    set_time_ms(0);
    eos_u32_t system_time = eos_port_time();

    // 测试订阅表的初始化 --------------------------------------------------------
    eventos_init();

#if (EOS_USE_PUB_SUB != 0)
    // 检查尚未设置订阅表
    TEST_ASSERT_EQUAL_INT8(EosRunErr_SubTableNull, eos_once());
    TEST_ASSERT_EQUAL_INT8(EosRunErr_SubTableNull, eos_event_pub_ret(Event_Test, EOS_NULL, 0));
    eos_sub_init(sub_table, Event_Max);
#endif

    fsm_init(&fsm, 0, EOS_NULL);
    TEST_ASSERT_EQUAL_UINT8(0, f->timer_count);
    ret = eos_event_pub_time(Event_Time_500ms, 500, EOS_True);
    TEST_ASSERT_EQUAL_UINT8(1, f->timer_count);
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, ret);
    for (int i = 0; i < 500; i ++) {
        set_time_ms(i);
        TEST_ASSERT_EQUAL_INT8(EosRun_NoEvent, eos_once());
    }
    set_time_ms(500);
    TEST_ASSERT_EQUAL_UINT8(1, f->timer_count);
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_once());
    TEST_ASSERT_EQUAL_UINT8(0, f->timer_count);
    TEST_ASSERT_EQUAL_UINT32(1, fsm_state(&fsm));

    // 再次发送500ms延时事件
    ret = eos_event_pub_time(Event_Time_500ms, 500, EOS_True);
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1, f->timer_count);
    system_time = eos_port_time();
    for (int i = system_time; i < (system_time + 500); i ++) {
        set_time_ms(i);
        TEST_ASSERT_EQUAL_INT8(EosRun_NoEvent, eos_once());
    }
    set_time_ms(system_time + 500);
    TEST_ASSERT_EQUAL_UINT8(1, f->timer_count);
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_once());
    TEST_ASSERT_EQUAL_UINT8(0, f->timer_count);
    TEST_ASSERT_EQUAL_UINT32(0, fsm_state(&fsm));
#endif
}
