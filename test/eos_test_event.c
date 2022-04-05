
/* include ------------------------------------------------------------------ */
#include "eos_test.h"
#include "eos_test_def.h"
#include "unity.h"
#include "unity_pack.h"

#if (EOS_USE_SM_MODE != 0)
/* unit test ---------------------------------------------------------------- */
static fsm_t fsm, fsm2;
static reactor_t reactor;
static eos_t *f;
#endif

/* eventos API for test ----------------------------------------------------- */
void eos_test_event(void)
{
#if (EOS_USE_SM_MODE != 0)
    int8_t ret;
    f = eos_get_framework();

    TEST_ASSERT_EQUAL_INT8(EosRunErr_NotInitEnd, eos_once());

    // 测试订阅表的初始化 --------------------------------------------------------
    eos_init();

    // 测试框架的停止
    eos_stop();
    TEST_ASSERT_EQUAL_INT8(EosRun_NotEnabled, eos_once());

    // 测试尚未注册Actor的情况
    eos_init();
    TEST_ASSERT_EQUAL_INT8(EosRun_NoActor, eos_event_pub_ret("Event_Test", EOS_NULL, 0));
    TEST_ASSERT_EQUAL_INT8(EosRun_NoActor, eos_once());

    // 注册Actor
    static uint8_t stack_fsm[1024];
    fsm_init(&fsm, "fsm", 0, stack_fsm, 1024);
    TEST_ASSERT_EQUAL_UINT32(1, f->task_exist);
    TEST_ASSERT_EQUAL_INT8(EosRun_NoEvent, eos_once());
#if (EOS_USE_PUB_SUB != 0)
    TEST_ASSERT_EQUAL_INT8(EosRun_NoActorSub, eos_event_pub_ret("Event_Test", EOS_NULL, 0));
    TEST_ASSERT_EQUAL_INT8(0, f->heap.sub_general);
    TEST_ASSERT_EQUAL_INT8(0, f->heap.count);
#else
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_event_pub_ret("Event_Test", EOS_NULL, 0));
    TEST_ASSERT_EQUAL_INT8(1, f->heap.count);
    TEST_ASSERT_EQUAL_INT8(1, f->heap.sub_general);
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_once());
    TEST_ASSERT_EQUAL_INT8(EosRun_NoEvent, eos_once());
    TEST_ASSERT_EQUAL_UINT8(1, f->heap.empty);
#endif

    #define EOS_EVENT_PUB_TIMES                     10
    // eos_event_pub_ret
    for (int i = 0; i < EOS_EVENT_PUB_TIMES; i ++) {
        TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_event_pub_ret("Event_TestFsm", EOS_NULL, 0));
        TEST_ASSERT_EQUAL_INT8((1 + i), f->heap.count);
        TEST_ASSERT_EQUAL_INT8(1, f->heap.sub_general);
        TEST_ASSERT_EQUAL_UINT32(0, fsm_state(&fsm));
        TEST_ASSERT_EQUAL_UINT32(0, fsm_event_count(&fsm));
    }

    uint32_t state;
    for (int i = 0; i < EOS_EVENT_PUB_TIMES; i ++) {
        state = (i % 2 == 1) ? 1 : 0;
        TEST_ASSERT_EQUAL_UINT32(state, fsm_state(&fsm));
        TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_once());
        state = (i % 2 == 0) ? 1 : 0;
        TEST_ASSERT_EQUAL_UINT32(state, fsm_state(&fsm));
        TEST_ASSERT_EQUAL_UINT32((1 + i), fsm_event_count(&fsm));

        TEST_ASSERT_EQUAL_INT8((EOS_EVENT_PUB_TIMES - 1 - i), f->heap.count);
    }

    TEST_ASSERT_EQUAL_UINT32(0, fsm_state(&fsm));
    TEST_ASSERT_EQUAL_INT8(EosRun_NoEvent, eos_once());
    TEST_ASSERT_EQUAL_UINT8(1, f->heap.empty);
    fsm_reset_event_count(&fsm);
    TEST_ASSERT_EQUAL_UINT32(0, fsm_event_count(&fsm));

    // 再次注册Actor
    static uint8_t stack_fsm2[1024];
    fsm_init(&fsm2, "fsm2", 1, stack_fsm2, sizeof(stack_fsm2));

    for (int i = 0; i < EOS_EVENT_PUB_TIMES; i ++) {
        TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_event_pub_ret("Event_TestFsm", EOS_NULL, 0));
        TEST_ASSERT_EQUAL_INT8((1 + i), f->heap.count);
        TEST_ASSERT_EQUAL_INT8(3, f->heap.sub_general);
        TEST_ASSERT_EQUAL_UINT32(0, fsm_state(&fsm));
        TEST_ASSERT_EQUAL_UINT32(0, fsm_event_count(&fsm));
    }

    TEST_ASSERT_EQUAL_UINT32(3, f->heap.sub_general);

    for (int i = 0; i < EOS_EVENT_PUB_TIMES; i ++) {
        state = (i % 2 == 1) ? 1 : 0;
        TEST_ASSERT_EQUAL_UINT32(state, fsm_state(&fsm2));
        TEST_ASSERT_EQUAL_UINT32(0, fsm_state(&fsm));
        TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_once());
        state = (i % 2 == 0) ? 1 : 0;
        TEST_ASSERT_EQUAL_UINT32(state, fsm_state(&fsm2));
        TEST_ASSERT_EQUAL_UINT32(0, fsm_state(&fsm));
        TEST_ASSERT_EQUAL_UINT32((1 + i), fsm_event_count(&fsm2));
        TEST_ASSERT_EQUAL_UINT32(0, fsm_event_count(&fsm));

        TEST_ASSERT_EQUAL_INT8(EOS_EVENT_PUB_TIMES, f->heap.count);
    }

    TEST_ASSERT_EQUAL_UINT32(1, f->heap.sub_general);

    for (int i = 0; i < EOS_EVENT_PUB_TIMES; i ++) {
        state = (i % 2 == 1) ? 1 : 0;
        TEST_ASSERT_EQUAL_UINT32(state, fsm_state(&fsm));
        TEST_ASSERT_EQUAL_UINT32(0, fsm_state(&fsm2));
        TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_once());
        state = (i % 2 == 0) ? 1 : 0;
        TEST_ASSERT_EQUAL_UINT32(state, fsm_state(&fsm));
        TEST_ASSERT_EQUAL_UINT32(0, fsm_state(&fsm2));
        TEST_ASSERT_EQUAL_UINT32((1 + i), fsm_event_count(&fsm));
        TEST_ASSERT_EQUAL_UINT32(EOS_EVENT_PUB_TIMES, fsm_event_count(&fsm2));

        TEST_ASSERT_EQUAL_INT8((EOS_EVENT_PUB_TIMES - 1 - i), f->heap.count);
    }

    TEST_ASSERT_EQUAL_UINT32(0, f->heap.sub_general);
    TEST_ASSERT_EQUAL_UINT32(EOS_SIZE_HEAP, f->heap.queue);

    TEST_ASSERT_EQUAL_INT8(EosRun_NoEvent, eos_once());
    TEST_ASSERT_EQUAL_UINT8(1, f->heap.empty);

    // 测试事件携带数据的功能
    uint8_t stack_reactor[1024];
    uint32_t event_value = 0;
    reactor_init(&reactor, "Reactor", 3, stack_reactor, 1024);
    // 发布事件，携带数据
    for (int i = 0; i < 32; i ++) {
        event_value = 100 * i;
        TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_event_pub_ret("Event_Data", &event_value, sizeof(uint32_t)));
        TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_once());
        TEST_ASSERT_EQUAL_UINT32(event_value, reactor_get_value(&reactor));
        TEST_ASSERT_EQUAL_INT8(EosRun_NoEvent, eos_once());
        TEST_ASSERT_EQUAL_UINT8(1, f->heap.empty);
    }
#endif
}
