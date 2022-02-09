#include "meow.h"
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include "stdio.h"

uint64_t port_get_time_ms(void)
{
    struct timeval time_crt;
    gettimeofday(&time_crt, (void *)0);
    uint64_t time_crt_ms = time_crt.tv_sec * 1000 + time_crt.tv_usec / 1000;

    return time_crt_ms;
}

void port_irq_disable(void)
{
    // NULL
}


void port_irq_enable(void)
{
    // NULL
}

void port_log(char * p_char)
{
    printf("%s", p_char);
    fflush(stdout);
}

void port_assert(char const * const module, int location)
{
    printf("Assert: %s, location: %d.\n", module, location);
    fflush(stdout);
    while (true)
        usleep(100000);
}

void hook_idle(void)
{
    usleep(10000);
}

void hook_start(void)
{

}

void hook_stop(void)
{

}

void * port_malloc(uint32_t size)
{
    return malloc(size);
}

void port_free(void * ptr)
{
    free(ptr);
}
