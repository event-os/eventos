
/* include ------------------------------------------------------------------ */
#include "eos_test.h"
#include "eos_test_def.h"
#include "unity.h"
#include "unity_pack.h"

static reactor_t reactor1, reactor2;
static eos_t *f;

void eos_test_reactor(void)
{
    f = eos_get_framework();
#if (EOS_USE_TIME_EVENT != 0)
    eos_set_time(0);
    uint32_t system_time = eos_time();
#endif

    eos_init();
    static uint8_t stack_reactor1[1024], stack_reactor2[1024];
    reactor_init(&reactor1, "reactor1", 1, stack_reactor1, sizeof(stack_reactor1));
    reactor_init(&reactor2, "reactor2", 2, stack_reactor2, sizeof(stack_reactor2));

    TEST_ASSERT_EQUAL_INT8(EosRun_NoEvent, eos_execute());

    // 发送Event_Test
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_event_pub_ret("Event_Test", EOS_NULL, 0));
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_execute());
    TEST_ASSERT_EQUAL_INT32(1, reactor_e_test_count(&reactor2));
    TEST_ASSERT_EQUAL_INT32(0, reactor_e_test_count(&reactor1));
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_execute());
    TEST_ASSERT_EQUAL_INT32(1, reactor_e_test_count(&reactor2));
    TEST_ASSERT_EQUAL_INT32(1, reactor_e_test_count(&reactor1));

    TEST_ASSERT_EQUAL_INT8(EosRun_NoEvent, eos_execute());

    // 发送Event_TestReactor
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_event_pub_ret("Event_TestReactor", EOS_NULL, 0));
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_execute());
    TEST_ASSERT_EQUAL_INT32(1, reactor_e_tr_count(&reactor2));
    TEST_ASSERT_EQUAL_INT32(0, reactor_e_tr_count(&reactor1));
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_execute());
    TEST_ASSERT_EQUAL_INT32(1, reactor_e_tr_count(&reactor2));
    TEST_ASSERT_EQUAL_INT32(1, reactor_e_tr_count(&reactor1));

    TEST_ASSERT_EQUAL_INT8(EosRun_NoEvent, eos_execute());

    uint8_t data[256];
    for (int i = 0; i < 256; i ++) {
        data[i] = i;
    }
    for (int i = 1; i < 256; i ++) {
        TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_event_pub_ret("Event_Test", data, i));
        TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_execute());
        TEST_ASSERT_EQUAL_INT32(i, reactor2.data_size);
        TEST_ASSERT_EQUAL_INT32(i - 1, reactor1.data_size);
        TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_execute());
        TEST_ASSERT_EQUAL_INT32(i, reactor2.data_size);
        TEST_ASSERT_EQUAL_INT32(i, reactor1.data_size);

        TEST_ASSERT_EQUAL_INT8(EosRun_NoEvent, eos_execute());
    }
}
