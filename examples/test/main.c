#include "eventos.h"
#include "unity.h"
#include "eos_test.h"

// config ----------------------------------------------------------------------
void setUp(void) {}
void tearDown(void) {}

extern void eos_test(void);

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(eos_test_heap);
    // RUN_TEST(eos_test_etimer);
    // RUN_TEST(eos_test_event);
    // RUN_TEST(eos_test_sm);
    // RUN_TEST(eos_test_reactor);
    // RUN_TEST(eos_test_sub);

    UNITY_END();

    return 0;
}
