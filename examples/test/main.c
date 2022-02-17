#include "eventos.h"
#include "unity.h"

// config ----------------------------------------------------------------------
void setUp(void) {}
void tearDown(void) {}

extern void eos_test(void);

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(eos_test);

    UNITY_END();

    return 0;
}
