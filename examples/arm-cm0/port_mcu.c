#include "meow.h"
#include "bsp.h"

uint64_t port_get_time_ms(void)
{
    uint64_t time_ms;
    port_irq_disable();
    time_ms = bsp_get_time_ms();
    port_irq_enable();
    
    return time_ms;
}

static volatile int _irq_level = 0;
void port_irq_disable(void)
{
    _irq_level --;
    _irq_level = _irq_level < 0 ? 0 : _irq_level;
    if (_irq_level == 0) {
        __disable_irq();
    }
}


void port_irq_enable(void)
{
    __enable_irq();
    _irq_level ++;
}

void port_assert(char const * const module, int location)
{
    while (1);
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
