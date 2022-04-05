
/* include ------------------------------------------------------------------ */
#include "eos_test.h"
#include "eos_test_def.h"
#include "unity.h"

/* unittest ----------------------------------------------------------------- */
#if (EOS_USE_SM_MODE != 0)
static fsm_t fsm, fsm2;
static eos_t *f;
#endif

void eos_test_sub(void)
{
#if (EOS_USE_SM_MODE != 0)
    int8_t ret;
    f = eos_get_framework();

    eos_init();

    // 注册Actor
    uint8_t stack_fsm[1024];
    fsm_init(&fsm, "fsm", 0, stack_fsm, sizeof(stack_fsm));
    TEST_ASSERT_EQUAL_UINT32(1, f->task_exist);
    TEST_ASSERT_EQUAL_INT8(EosRun_NoEvent, eos_execute());
#if (EOS_USE_PUB_SUB != 0)
    TEST_ASSERT_EQUAL_INT8(EosRun_NoActorSub, eos_event_pub_ret("Event_Test", EOS_NULL, 0));
    TEST_ASSERT_EQUAL_INT8(0, f->heap.sub_general);
    TEST_ASSERT_EQUAL_INT8(0, f->heap.count);
#else
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_event_pub_ret("Event_Test", EOS_NULL, 0));
    TEST_ASSERT_EQUAL_INT8(1, f->heap.count);
    TEST_ASSERT_EQUAL_INT8(1, f->heap.sub_general);
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_execute());
    TEST_ASSERT_EQUAL_INT8(EosRun_NoEvent, eos_execute());
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
        TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_execute());
        state = (i % 2 == 0) ? 1 : 0;
        TEST_ASSERT_EQUAL_UINT32(state, fsm_state(&fsm));
        TEST_ASSERT_EQUAL_UINT32((1 + i), fsm_event_count(&fsm));

        TEST_ASSERT_EQUAL_INT8((EOS_EVENT_PUB_TIMES - 1 - i), f->heap.count);
    }

    TEST_ASSERT_EQUAL_UINT32(0, fsm_state(&fsm));
    TEST_ASSERT_EQUAL_INT8(EosRun_NoEvent, eos_execute());
    TEST_ASSERT_EQUAL_UINT8(1, f->heap.empty);
    fsm_reset_event_count(&fsm);
    TEST_ASSERT_EQUAL_UINT32(0, fsm_event_count(&fsm));

    // 再次注册Actor
    uint8_t stack_fsm2[1024];
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
        TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_execute());
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
        TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_execute());
        state = (i % 2 == 0) ? 1 : 0;
        TEST_ASSERT_EQUAL_UINT32(state, fsm_state(&fsm));
        TEST_ASSERT_EQUAL_UINT32(0, fsm_state(&fsm2));
        TEST_ASSERT_EQUAL_UINT32((1 + i), fsm_event_count(&fsm));
        TEST_ASSERT_EQUAL_UINT32(EOS_EVENT_PUB_TIMES, fsm_event_count(&fsm2));

        TEST_ASSERT_EQUAL_INT8((EOS_EVENT_PUB_TIMES - 1 - i), f->heap.count);
    }

    TEST_ASSERT_EQUAL_UINT32(0, f->heap.sub_general);
    TEST_ASSERT_EQUAL_UINT32(EOS_SIZE_HEAP, f->heap.queue);

    TEST_ASSERT_EQUAL_INT8(EosRun_NoEvent, eos_execute());
    TEST_ASSERT_EQUAL_UINT8(1, f->heap.empty);

#if (EOS_USE_PUB_SUB != 0)
    // 测试取消订阅
    eos_event_unsub(&fsm2.super.super, "Event_TestFsm");
    for (int i = 0; i < EOS_EVENT_PUB_TIMES; i ++) {
        TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_event_pub_ret("Event_TestFsm", EOS_NULL, 0));
        TEST_ASSERT_EQUAL_INT8((1 + i), f->heap.count);
        TEST_ASSERT_EQUAL_INT8(1, f->heap.sub_general);
        TEST_ASSERT_EQUAL_UINT32(0, fsm_state(&fsm));
        TEST_ASSERT_EQUAL_UINT32(EOS_EVENT_PUB_TIMES, fsm_event_count(&fsm));
        TEST_ASSERT_EQUAL_UINT8(0, f->heap.empty);
    }

    TEST_ASSERT_EQUAL_UINT32(1, f->heap.sub_general);

    for (int i = 0; i < EOS_EVENT_PUB_TIMES; i ++) {
        state = (i % 2 == 1) ? 1 : 0;
        TEST_ASSERT_EQUAL_UINT32(0, fsm_state(&fsm2));
        TEST_ASSERT_EQUAL_UINT32(state, fsm_state(&fsm));
        TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_execute());
        state = (i % 2 == 0) ? 1 : 0;
        TEST_ASSERT_EQUAL_UINT32(0, fsm_state(&fsm2));
        TEST_ASSERT_EQUAL_UINT32(state, fsm_state(&fsm));
        TEST_ASSERT_EQUAL_UINT32(10, fsm_event_count(&fsm2));
        TEST_ASSERT_EQUAL_UINT32((11 + i), fsm_event_count(&fsm));

        TEST_ASSERT_EQUAL_INT8((EOS_EVENT_PUB_TIMES - i - 1), f->heap.count);
    }

    TEST_ASSERT_EQUAL_UINT32(0, f->heap.sub_general);
    TEST_ASSERT_EQUAL_UINT32(EOS_SIZE_HEAP, f->heap.queue);

    TEST_ASSERT_EQUAL_INT8(EosRun_NoEvent, eos_execute());
    TEST_ASSERT_EQUAL_UINT8(1, f->heap.empty);

    // 测试取消订阅
    eos_event_unsub(&fsm.super.super, "Event_TestFsm");

    for (int i = 0; i < EOS_EVENT_PUB_TIMES; i ++) {
        TEST_ASSERT_EQUAL_INT8(EosRun_NoActorSub, eos_event_pub_ret("Event_TestFsm", EOS_NULL, 0));
        TEST_ASSERT_EQUAL_INT8(0, f->heap.count);
        TEST_ASSERT_EQUAL_INT8(0, f->heap.sub_general);
        TEST_ASSERT_EQUAL_UINT32(0, fsm_state(&fsm));
        TEST_ASSERT_EQUAL_UINT32(EOS_EVENT_PUB_TIMES * 2, fsm_event_count(&fsm));
        TEST_ASSERT_EQUAL_UINT8(1, f->heap.empty);
    }

    TEST_ASSERT_EQUAL_UINT32(0, f->heap.sub_general);
    TEST_ASSERT_EQUAL_UINT32(EOS_SIZE_HEAP, f->heap.queue);

    TEST_ASSERT_EQUAL_INT8(EosRun_NoEvent, eos_execute());
    TEST_ASSERT_EQUAL_UINT8(1, f->heap.empty);
#endif
#endif
}
