
#include "chip.h"
#include "core_cm4.h"

static void sys_tick_init(uint32_t ticks);

uint64_t time_ms_count = 0;
uint64_t bsp_get_time_ms(void) {
    return time_ms_count;
}

void SystemInit(void)
{
    Chip_SYSCTL_EnableBoost();
    Chip_SetupXtalClocking();
    Chip_Clock_SetSPIFIClockDiv(2);
    Chip_Clock_SetSPIFIClockSource(SYSCTL_SPIFICLKSRC_MAINPLL);
}

void bsp_init(void)
{
    SystemCoreClockUpdate();
    sys_tick_init(120000000 / 1000);
}

static void sys_tick_init(uint32_t ticks)
{
    SysTick->LOAD  = (uint32_t)(ticks - 1UL);
    NVIC_SetPriority (SysTick_IRQn, 0);
    SysTick->VAL = 0UL;
}

void systick_enable(void)
{
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk |
                    SysTick_CTRL_ENABLE_Msk;
}

void vPortSVCHandler(void)
{
    
}


void xPortSysTickHandler(void)
{
    time_ms_count ++;
}

void xPortPendSVHandler(void)
{
    
}

void HardFault_Handler(void)
{

}
