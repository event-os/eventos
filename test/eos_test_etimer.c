
#include "eos_test.h"
#include "event_def.h"
#include "unity.h"
#include "unity_pack.h"
#include "eos_test_def.h"

#if (EOS_USE_SM_MODE != 0)
#if (EOS_USE_TIME_EVENT != 0)
/* unit test ---------------------------------------------------------------- */
static eos_mcu_t sub_table[Event_Max];
static fsm_t fsm;
static eos_t *f;
#endif
#endif

void eos_test_etimer(void)
{
#if (EOS_USE_SM_MODE != 0)
#if (EOS_USE_TIME_EVENT != 0)
    f = eos_get_framework();
    eos_set_time(0);
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
        eos_set_time(i);
        TEST_ASSERT_EQUAL_INT8(EosRun_NoEvent, eos_once());
    }
    eos_set_time(500);
    TEST_ASSERT_EQUAL_UINT8(1, f->timer_count);
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_once());
    TEST_ASSERT_EQUAL_UINT8(0, f->timer_count);
    TEST_ASSERT_EQUAL_UINT32(1, fsm_state(&fsm));

    // 再次发送500ms延时事件 ----------------------------------------------------
    eos_event_pub_delay(Event_Time_500ms, 500);
    TEST_ASSERT_EQUAL_UINT8(1, f->timer_count);
    system_time = eos_time();
    for (int i = system_time; i < (system_time + 500); i ++) {
        eos_set_time(i);
        TEST_ASSERT_EQUAL_INT8(EosRun_NoEvent, eos_once());
    }
    eos_set_time(system_time + 500);
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

    eos_set_time(system_time + 500);
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
    eos_set_time(system_time + 70000);
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
    eos_set_time(system_time + time_2hour);
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
    eos_set_time(system_time + time_17hour);
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_once());
    TEST_ASSERT_EQUAL_UINT8(0, f->timer_count);

    // 测试时间溢出
    eos_set_time(EOS_MS_NUM_30DAY - 100);
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
    eos_set_time(400);
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_once());
    TEST_ASSERT_EQUAL_UINT8(1, f->timer_count);
    TEST_ASSERT_EQUAL_UINT32((17 * 60), f->etimer[0].period);
    eos_set_time(time_17hour - 100);
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_once());
    TEST_ASSERT_EQUAL_UINT8(0, f->timer_count);

    // 对周期事件进行单元测试
    eos_set_time(0);
    system_time = eos_time();
    eos_event_pub_period(Event_Time_500ms, 500);
    eos_event_pub_period(Event_TestFsm, 1000);
    TEST_ASSERT_EQUAL_UINT32(500, f->etimer[0].period);
    TEST_ASSERT_EQUAL_UINT32((system_time + 500), f->etimer[0].timeout_ms);
    TEST_ASSERT_EQUAL_UINT32(0, f->etimer[0].oneshoot);
    TEST_ASSERT_EQUAL_UINT32(1000, f->etimer[1].period);
    TEST_ASSERT_EQUAL_UINT32((system_time + 1000), f->etimer[1].timeout_ms);
    TEST_ASSERT_EQUAL_UINT32(0, f->etimer[1].oneshoot);
    TEST_ASSERT_EQUAL_UINT8(2, f->timer_count);
    eos_set_time(500);
    system_time = eos_time();
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_once());
    TEST_ASSERT_EQUAL_INT8(EosRun_NoEvent, eos_once());
    TEST_ASSERT_EQUAL_UINT8(2, f->timer_count);
    TEST_ASSERT_EQUAL_UINT32(500, f->etimer[0].period);
    TEST_ASSERT_EQUAL_UINT32((system_time + 500), f->etimer[0].timeout_ms);
    TEST_ASSERT_EQUAL_UINT32(1000, f->etimer[1].period);
    TEST_ASSERT_EQUAL_UINT32((system_time + 500), f->etimer[1].timeout_ms);
    eos_set_time(1000);
    system_time = eos_time();
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_once());
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_once());
    TEST_ASSERT_EQUAL_INT8(EosRun_NoEvent, eos_once());
    TEST_ASSERT_EQUAL_UINT8(2, f->timer_count);
    eos_set_time(1500);
    system_time = eos_time();
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_once());
    TEST_ASSERT_EQUAL_INT8(EosRun_NoEvent, eos_once());
    TEST_ASSERT_EQUAL_UINT8(2, f->timer_count);
    TEST_ASSERT_EQUAL_UINT32(500, f->etimer[0].period);
    TEST_ASSERT_EQUAL_UINT32((system_time + 500), f->etimer[0].timeout_ms);
    TEST_ASSERT_EQUAL_UINT32(1000, f->etimer[1].period);
    TEST_ASSERT_EQUAL_UINT32((system_time + 500), f->etimer[1].timeout_ms);
    eos_set_time(2000);
    system_time = eos_time();
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_once());
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_once());
    TEST_ASSERT_EQUAL_INT8(EosRun_NoEvent, eos_once());
    TEST_ASSERT_EQUAL_UINT8(2, f->timer_count);

    // 对取消周期事件进行单元测试
    eos_set_time(2300);
    system_time = eos_time();
    eos_event_time_cancel(Event_Time_500ms);
    TEST_ASSERT_EQUAL_UINT8(1, f->timer_count);
    TEST_ASSERT_EQUAL_UINT32(1000, f->etimer[0].period);
    TEST_ASSERT_EQUAL_UINT32((system_time + 700), f->etimer[0].timeout_ms);
    eos_event_time_cancel(Event_TestFsm);
    TEST_ASSERT_EQUAL_UINT8(0, f->timer_count);
#endif
#endif
}
