
#include "eos_test.h"
#include "event_def.h"
#include "unity.h"
#include "unity_pack.h"

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
    eos_u32_t unit                          : 2;
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
eos_s32_t eos_evttimer(void);
#endif

void eos_test_etimer(void)
{
#if (EOS_USE_TIME_EVENT != 0)
    f = eos_get_framework();
    set_time_ms(0);
    eos_u32_t system_time = eos_time();

    // 测试订阅表的初始化 --------------------------------------------------------
    eos_init();

#if (EOS_USE_PUB_SUB != 0)
    // 检查尚未设置订阅表
    TEST_ASSERT_EQUAL_INT8(EosRunErr_SubTableNull, eos_once());
    TEST_ASSERT_EQUAL_INT8(EosRunErr_SubTableNull, eos_event_pub_ret(Event_Test, EOS_NULL, 0));
    eos_sub_init(sub_table, Event_Max);
#endif

    fsm_init(&fsm, 0, EOS_NULL);

    // 发送500ms延时事件 --------------------------------------------------------
    TEST_ASSERT_EQUAL_UINT8(0, f->timer_count);
    eos_event_pub_delay(Event_Time_500ms, 500);
    TEST_ASSERT_EQUAL_UINT8(1, f->timer_count);
    for (int i = 0; i < 500; i ++) {
        set_time_ms(i);
        TEST_ASSERT_EQUAL_INT8(EosRun_NoEvent, eos_once());
    }
    set_time_ms(500);
    TEST_ASSERT_EQUAL_UINT8(1, f->timer_count);
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_once());
    TEST_ASSERT_EQUAL_UINT8(0, f->timer_count);
    TEST_ASSERT_EQUAL_UINT32(1, fsm_state(&fsm));

    // 再次发送500ms延时事件 ----------------------------------------------------
    eos_event_pub_delay(Event_Time_500ms, 500);
    TEST_ASSERT_EQUAL_UINT8(1, f->timer_count);
    system_time = eos_time();
    for (int i = system_time; i < (system_time + 500); i ++) {
        set_time_ms(i);
        TEST_ASSERT_EQUAL_INT8(EosRun_NoEvent, eos_once());
    }
    set_time_ms(system_time + 500);
    TEST_ASSERT_EQUAL_UINT8(1, f->timer_count);
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_once());
    TEST_ASSERT_EQUAL_UINT8(0, f->timer_count);
    TEST_ASSERT_EQUAL_UINT32(0, fsm_state(&fsm));
 
    // 发送两个时间事件，测试取消功能 ---------------------------------------------
    system_time = eos_time();
    TEST_ASSERT_EQUAL_UINT8(0, f->timer_count);
    eos_event_pub_delay(Event_Time_500ms, 500);
    TEST_ASSERT_EQUAL_UINT8(1, f->timer_count);
    eos_event_pub_delay(Event_Test, 1000);
    TEST_ASSERT_EQUAL_UINT8(2, f->timer_count);
    TEST_ASSERT_EQUAL_UINT16(Event_Time_500ms, f->etimer[0].topic);
    TEST_ASSERT_EQUAL_UINT8(1, f->etimer[0].oneshoot);
    TEST_ASSERT_EQUAL_UINT32((system_time + 500), f->etimer[0].timeout_ms);
    TEST_ASSERT_EQUAL_UINT8(EosTimerUnit_Ms, f->etimer[0].unit);
    TEST_ASSERT_EQUAL_UINT16(Event_Test, f->etimer[1].topic);
    TEST_ASSERT_EQUAL_UINT8(1, f->etimer[1].oneshoot);
    TEST_ASSERT_EQUAL_UINT32((system_time + 1000), f->etimer[1].timeout_ms);
    TEST_ASSERT_EQUAL_UINT8(EosTimerUnit_Ms, f->etimer[1].unit);

    // 取消延时事件
    eos_event_time_cancel(Event_Time_500ms);
    TEST_ASSERT_EQUAL_UINT16(Event_Test, f->etimer[0].topic);
    TEST_ASSERT_EQUAL_UINT32((system_time + 1000), f->etimer[0].timeout_ms);
    TEST_ASSERT_EQUAL_UINT8(EosTimerUnit_Ms, f->etimer[0].unit);
    TEST_ASSERT_EQUAL_UINT8(1, f->timer_count);
    // 重复取消，并不管用
    eos_event_time_cancel(Event_Time_500ms);
    TEST_ASSERT_EQUAL_UINT16(Event_Test, f->etimer[0].topic);
    TEST_ASSERT_EQUAL_UINT32((system_time + 1000), f->etimer[0].timeout_ms);
    TEST_ASSERT_EQUAL_UINT8(EosTimerUnit_Ms, f->etimer[0].unit);
    TEST_ASSERT_EQUAL_UINT8(1, f->timer_count);
    // 取消另一个延时事件
    eos_event_time_cancel(Event_Test);
    TEST_ASSERT_EQUAL_UINT8(0, f->timer_count);

    // 发送两个时间事件，测试一个到期后另一个的移位 --------------------------------
    system_time = eos_time();
    TEST_ASSERT_EQUAL_UINT8(0, f->timer_count);
    eos_event_pub_delay(Event_Time_500ms, 500);
    TEST_ASSERT_EQUAL_UINT8(1, f->timer_count);
    eos_event_pub_delay(Event_Test, 1000);
    TEST_ASSERT_EQUAL_UINT8(2, f->timer_count);

    set_time_ms(system_time + 500);
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_once());
    TEST_ASSERT_EQUAL_UINT8(1, f->timer_count);
    TEST_ASSERT_EQUAL_UINT32(1, fsm_state(&fsm));
    TEST_ASSERT_EQUAL_UINT16(Event_Test, f->etimer[0].topic);
    TEST_ASSERT_EQUAL_UINT8(EosTimerUnit_Ms, f->etimer[0].unit);
    TEST_ASSERT_EQUAL_UINT32((system_time + 1000), f->etimer[0].timeout_ms);
    eos_event_time_cancel(Event_Test);
    TEST_ASSERT_EQUAL_UINT8(0, f->timer_count);

    // 延时70000毫秒的时间
    system_time = eos_time();
    TEST_ASSERT_EQUAL_UINT8(0, f->timer_count);
    eos_event_pub_delay(Event_TestFsm, 70000);
    TEST_ASSERT_EQUAL_UINT8(1, f->timer_count);
    TEST_ASSERT_EQUAL_UINT16(Event_TestFsm, f->etimer[0].topic);
    TEST_ASSERT_EQUAL_UINT32((system_time + 70000), f->etimer[0].timeout_ms);
    TEST_ASSERT_EQUAL_UINT32(EosTimerUnit_100Ms, f->etimer[0].unit);
    TEST_ASSERT_EQUAL_UINT32(700, f->etimer[0].period);
    TEST_ASSERT_EQUAL_UINT32(system_time + 70000, f->timeout_min);
    set_time_ms(system_time + 70000);
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_once());
    TEST_ASSERT_EQUAL_UINT8(0, f->timer_count);

    // 延时2小时的时间
    eos_u32_t time_2hour = (2 * 3600 * 1000);
    system_time = eos_time();
    TEST_ASSERT_EQUAL_UINT8(0, f->timer_count);
    eos_event_pub_delay(Event_TestFsm, time_2hour);
    TEST_ASSERT_EQUAL_UINT8(1, f->timer_count);
    TEST_ASSERT_EQUAL_UINT16(Event_TestFsm, f->etimer[0].topic);
    TEST_ASSERT_EQUAL_UINT32((system_time + time_2hour), f->etimer[0].timeout_ms);
    TEST_ASSERT_EQUAL_UINT32(EosTimerUnit_Sec, f->etimer[0].unit);
    TEST_ASSERT_EQUAL_UINT32(7200, f->etimer[0].period);
    TEST_ASSERT_EQUAL_UINT32(system_time + time_2hour, f->timeout_min);
    set_time_ms(system_time + time_2hour);
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_once());
    TEST_ASSERT_EQUAL_UINT8(0, f->timer_count);

    // 延时17小时的时间
    eos_u32_t time_17hour = (17 * 3600 * 1000);
    system_time = eos_time();
    TEST_ASSERT_EQUAL_UINT8(0, f->timer_count);
    eos_event_pub_delay(Event_TestFsm, time_17hour);
    TEST_ASSERT_EQUAL_UINT8(1, f->timer_count);
    TEST_ASSERT_EQUAL_UINT16(Event_TestFsm, f->etimer[0].topic);
    TEST_ASSERT_EQUAL_UINT32((system_time + time_17hour), f->etimer[0].timeout_ms);
    TEST_ASSERT_EQUAL_UINT32(EosTimerUnit_Minute, f->etimer[0].unit);
    TEST_ASSERT_EQUAL_UINT32((17 * 60), f->etimer[0].period);
    TEST_ASSERT_EQUAL_UINT32(system_time + time_17hour, f->timeout_min);
    set_time_ms(system_time + time_17hour);
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_once());
    TEST_ASSERT_EQUAL_UINT8(0, f->timer_count);

    // 测试时间溢出
    set_time_ms(EOS_MS_NUM_30DAY - 100);
    system_time = eos_time();
    eos_event_pub_delay(Event_Time_500ms, 500);
    eos_event_pub_delay(Event_TestFsm, time_17hour);
    TEST_ASSERT_EQUAL_UINT32(500, f->etimer[0].period);
    TEST_ASSERT_EQUAL_UINT32((system_time + 500), f->etimer[0].timeout_ms);
    TEST_ASSERT_EQUAL_UINT32((17 * 60), f->etimer[1].period);
    TEST_ASSERT_EQUAL_UINT32((system_time + time_17hour), f->etimer[1].timeout_ms);
    TEST_ASSERT_EQUAL_UINT8(2, f->timer_count);
    set_time_ms(0);
    TEST_ASSERT_EQUAL_UINT32(500, f->etimer[0].period);
    TEST_ASSERT_EQUAL_UINT32(400, f->etimer[0].timeout_ms);
    TEST_ASSERT_EQUAL_UINT32((17 * 60), f->etimer[1].period);
    TEST_ASSERT_EQUAL_UINT32((time_17hour - 100), f->etimer[1].timeout_ms);
    TEST_ASSERT_EQUAL_UINT8(2, f->timer_count);
    set_time_ms(400);
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_once());
    TEST_ASSERT_EQUAL_UINT8(1, f->timer_count);
    TEST_ASSERT_EQUAL_UINT32((17 * 60), f->etimer[0].period);
    set_time_ms(time_17hour - 100);
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_once());
    TEST_ASSERT_EQUAL_UINT8(0, f->timer_count);

#endif
}
