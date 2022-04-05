/* include ------------------------------------------------------------------ */
#include "stm32f4xx.h"
#include "eventos.h"                                // EventOS Nano头文件
#include "eos_led.h"                                // LED灯闪烁状态机

/* main function ------------------------------------------------------------ */
int main(void)
{
    if (SysTick_Config(SystemCoreClock / 1000) != 0)
        while (1);
    
    eos_init();                                     // EventOS初始化

#if (EOS_USE_SM_MODE != 0)
    eos_sm_led_init();                              // LED状态机初始化
#endif
    eos_reactor_led_init();

    eos_run();                                      // EventOS启动

    return 0;
}

void SysTick_Handler(void)
{
    eos_tick();
}

void HardFault_Handler(void)
{
    while (1) {
    }
}
