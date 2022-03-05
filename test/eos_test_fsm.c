
/* include ------------------------------------------------------------------ */
#include "eos_test.h"
#include "eventos.h"
#include "event_def.h"
#include "unity.h"
#include <stdbool.h>
#include "eos_test_def.h"

/* unittest ----------------------------------------------------------------- */
#if (EOS_USE_PUB_SUB != 0)
static eos_mcu_t eos_sub_table[Event_Max];
#endif
#if (EOS_USE_HSM_MODE == 0)
static fsm_t fsm, fsm2;
static eos_t *f;
#endif

void eos_test_fsm(void)
{
#if (EOS_USE_HSM_MODE == 0)
    eos_s8_t ret;
    f = eos_get_framework();

    // 测试尚未注册Actor的情况
    eos_init();
#if (EOS_USE_PUB_SUB != 0)
    eos_sub_init(eos_sub_table, Event_Max);
#endif
    TEST_ASSERT_EQUAL_INT8(EosRun_NoActor, eos_event_pub_ret(Event_Test, EOS_NULL, 0));
    TEST_ASSERT_EQUAL_INT8(EosRun_NoActor, eos_once());

    // 注册Actor
    fsm_init(&fsm, 0, EOS_NULL);
    TEST_ASSERT_EQUAL_UINT32(1, f->actor_exist);
    TEST_ASSERT_EQUAL_INT8(EosRun_NoEvent, eos_once());
#if (EOS_USE_PUB_SUB != 0)
    TEST_ASSERT_EQUAL_INT8(EosRun_NoActorSub, eos_event_pub_ret(Event_Test, EOS_NULL, 0));
    TEST_ASSERT_EQUAL_INT8(0, f->heap.sub_general);
    TEST_ASSERT_EQUAL_INT8(0, f->heap.count);
#else
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_event_pub_ret(Event_Test, EOS_NULL, 0));
    TEST_ASSERT_EQUAL_INT8(1, f->heap.count);
    TEST_ASSERT_EQUAL_INT8(1, f->heap.sub_general);
    TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_once());
    TEST_ASSERT_EQUAL_INT8(EosRun_NoEvent, eos_once());
    TEST_ASSERT_EQUAL_UINT8(1, f->heap.empty);
#endif

    #define EOS_EVENT_PUB_TIMES                     10
    // eos_event_pub_ret
    for (int i = 0; i < EOS_EVENT_PUB_TIMES; i ++) {
        TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_event_pub_ret(Event_TestFsm, EOS_NULL, 0));
        TEST_ASSERT_EQUAL_INT8((1 + i), f->heap.count);
        TEST_ASSERT_EQUAL_INT8(1, f->heap.sub_general);
        TEST_ASSERT_EQUAL_UINT32(0, fsm_state(&fsm));
        TEST_ASSERT_EQUAL_UINT32(0, fsm_event_count(&fsm));
    }

    eos_u32_t state;
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
    fsm_init(&fsm2, 1, EOS_NULL);

    for (int i = 0; i < EOS_EVENT_PUB_TIMES; i ++) {
        TEST_ASSERT_EQUAL_INT8(EosRun_OK, eos_event_pub_ret(Event_TestFsm, EOS_NULL, 0));
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
    TEST_ASSERT_EQUAL_UINT32(EOS_HEAP_MAX, f->heap.queue);

    TEST_ASSERT_EQUAL_INT8(EosRun_NoEvent, eos_once());
    TEST_ASSERT_EQUAL_UINT8(1, f->heap.empty);
#endif
}


