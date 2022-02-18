#include "eventos.h"
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

eos_u32_t eos_port_get_time_ms(void)
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

void eos_hook_idle(void)
{
    usleep(1000);
}

void eos_hook_start(void)
{

}

void eos_hook_stop(void)
{

}
