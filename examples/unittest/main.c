#include "eventos.h"
#include "stdio.h"
#include "unity.h"

// config ----------------------------------------------------------------------
void setUp(void) {}
void tearDown(void) {}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    UNITY_BEGIN();

    RUN_TEST(meow_unittest_sm);

    UNITY_END();

    return 0;
}
