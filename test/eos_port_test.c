#include "eventos.h"
#include <unistd.h>
#include "stdio.h"
#include <stdlib.h>

void set_time_ms(uint32_t time_ms)
{
#if (EOS_USE_TIME_EVENT != 0)
    #define EOS_MS_NUM_30DAY                    (2592000000)
    uint32_t time_ms_count = eos_time();

    if (time_ms >= time_ms_count) {
        for (uint32_t i = 0; i < (time_ms - time_ms_count); i ++)
            eos_tick();
    }
    else {
        uint32_t count = (EOS_MS_NUM_30DAY + time_ms - time_ms_count);
        for (uint32_t i = 0; i < count; i ++)
            eos_tick();
    }

    time_ms_count = time_ms;
#endif
}

void eos_port_critical_enter(void)
{
    // NULL
}

void eos_port_critical_exit(void)
{
    // NULL
}

void eos_hook_idle(void)
{

}

void eos_hook_start(void)
{

}

void eos_hook_stop(void)
{

}

void eos_port_assert(uint32_t error_id)
{
    printf("------------------------------------\n");
    printf("ASSERT >>> Module: EventOS Nano, ErrorId: %d.\n", error_id);
    printf("------------------------------------\n");

    while (1) {
        usleep(100000);
    }
}
