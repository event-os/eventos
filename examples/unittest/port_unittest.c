#include "eventos.h"
#include <sys/time.h>
#include <unistd.h>
#include "stdio.h"
#include <stdlib.h>

static eos_u32_t time_ms_count = 0;
void set_time_ms(eos_u32_t time_ms)
{
    time_ms_count = time_ms;
}

eos_u32_t eos_port_get_time_ms(void)
{
    return time_ms_count;
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

void * port_malloc(eos_u32_t size, const char * name)
{
    return malloc(size);
}
