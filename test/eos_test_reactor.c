
/* include ------------------------------------------------------------------ */
#include "eos_test.h"
#include "eos_test_def.h"
#include "event_def.h"
#include "unity.h"
#include "unity_pack.h"

#if (EOS_USE_PUB_SUB != 0)
static eos_mcu_t sub_table[Event_Max];
#endif
static reactor_t reactor1, reactor2;
static eos_t *f;

void eos_test_reactor(void)
{
    f = eos_get_framework();
#if (EOS_USE_TIME_EVENT != 0)
    eos_set_time(0);
    eos_u32_t system_time = eos_time();
#endif

    eos_init();
#if (EOS_USE_PUB_SUB != 0)
    eos_sub_init(sub_table, Event_Max);
#endif
    reactor_init(&reactor1, 1, EOS_NULL);
    reactor_init(&reactor2, 2, EOS_NULL);

    TEST_ASSERT_EQUAL_INT8(EosRun_NoEvent, eos_once());

    // 发送Event_Test
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_event_pub_ret(Event_Test, EOS_NULL, 0));
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_once());
    TEST_ASSERT_EQUAL_INT32(1, reactor_e_test_count(&reactor2));
    TEST_ASSERT_EQUAL_INT32(0, reactor_e_test_count(&reactor1));
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_once());
    TEST_ASSERT_EQUAL_INT32(1, reactor_e_test_count(&reactor2));
    TEST_ASSERT_EQUAL_INT32(1, reactor_e_test_count(&reactor1));

    TEST_ASSERT_EQUAL_INT8(EosRun_NoEvent, eos_once());

    // 发送Event_TestReactor
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_event_pub_ret(Event_TestReactor, EOS_NULL, 0));
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_once());
    TEST_ASSERT_EQUAL_INT32(1, reactor_e_tr_count(&reactor2));
    TEST_ASSERT_EQUAL_INT32(0, reactor_e_tr_count(&reactor1));
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_once());
    TEST_ASSERT_EQUAL_INT32(1, reactor_e_tr_count(&reactor2));
    TEST_ASSERT_EQUAL_INT32(1, reactor_e_tr_count(&reactor1));
}
