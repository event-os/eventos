#include "eventos.h"
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

static eos_u32_t eos_get_time(void)
{
    struct timeval time_crt;
    gettimeofday(&time_crt, (void *)0);
    eos_u32_t time_crt_ms = time_crt.tv_sec * 1000 + time_crt.tv_usec / 1000;

    return time_crt_ms;
}

void eos_port_critical_enter(void)
{
    // NULL
}


void eos_port_critical_exit(void)
{
    // NULL
}

void eos_port_assert(eos_u32_t error_id)
{
    printf("------------------------------------\n");
    printf("ASSERT >>> Module: EventOS Nano, ErrorId: %d.\n", error_id);
    printf("------------------------------------\n");

    while (1) {
        usleep(100000);
    }
}

static eos_u32_t eos_time_bkp = 0;
void eos_hook_idle(void)
{
#if (EOS_USE_TIME_EVENT != 0)
    if (eos_time_bkp == 0) {
        eos_time_bkp = eos_get_time();
        usleep(1000);
        return;
    }

    eos_u32_t system_time = eos_get_time();

    if (system_time >= eos_time_bkp) {
        for (eos_u32_t i = 0; i < (system_time - eos_time_bkp); i ++) {
            eos_tick();
        }
        eos_time_bkp = system_time;
    }
    // TODO 此处需要处理时间溢出情况。
#endif

    usleep(1000);
}

void eos_hook_start(void)
{

}

void eos_hook_stop(void)
{

}
