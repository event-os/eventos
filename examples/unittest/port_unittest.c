#include "eventos.h"
#include <sys/time.h>
#include <unistd.h>
#include "stdio.h"
#include <stdlib.h>

static uint64_t time_ms_count = 0;
void set_time_ms(uint64_t time_ms)
{
    time_ms_count = time_ms;
}

uint64_t port_get_time_ms(void)
{
    return time_ms_count;
}

void port_critical_enter(void)
{
    // NULL
}

void port_critical_exit(void)
{
    // NULL
}

void hook_idle(void)
{

}

void hook_start(void)
{

}

void hook_stop(void)
{

}

void * port_malloc(uint32_t size, const char * name)
{
    return malloc(size);
}
