#include "eventos.h"
#include "rtt/SEGGER_RTT.h"

void eos_port_critical_enter(void)
{
    __disable_irq();
}


void eos_port_critical_exit(void)
{
    __enable_irq();
}

eos_u32_t eos_error_id = 0;
void eos_port_assert(eos_u32_t error_id)
{
    SEGGER_RTT_printf(0, "------------------------------------\n");
    SEGGER_RTT_printf(0, "ASSERT >>> Module: EventOS Nano, ErrorId: %d.\n", error_id);
    SEGGER_RTT_printf(0, "------------------------------------\n");
    
    eos_error_id = error_id;

    while (1) {
    }
}

void eos_hook_idle(void)
{
}

void eos_hook_start(void)
{
    SEGGER_RTT_Init();
}

void eos_hook_stop(void)
{

}
