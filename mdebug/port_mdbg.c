
#include "m_debug.h"
#include "stdio.h"
#include <sys/time.h>
#include <unistd.h>
#include "stdlib.h"

void port_dbg_init(void)
{

}

void port_dbg_out(char * p_char)
{
    printf("%s", p_char);
    fflush(stdout);
}

uint64_t port_sys_time_ms(void)
{
    struct timeval time_crt;
    gettimeofday(&time_crt, (void *)0);
    uint64_t time_crt_ms = time_crt.tv_sec * 1000 + time_crt.tv_usec / 1000;

    return time_crt_ms;
}

void port_sassert_out(char * p_char)
{
    printf("%s", p_char);
}

void port_after_assert(void)
{
    while (1) {
        usleep(10000);
    }
}

void * port_dbg_malloc(int size)
{
    return malloc(size);
}