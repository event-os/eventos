/* include ------------------------------------------------------------------ */
#include "stm32f4xx.h"
#include "eventos.h"                                // EventOS Nano头文件
#include "eos_led.h"                                // LED灯闪烁状态机

/* main function ------------------------------------------------------------ */
uint8_t stack_task[1024];
eos_task_t task;
uint32_t count_test = 0;

void task_func_test(void *parameter)
{
    (void)parameter;
    
    while (1) {
        count_test ++;
        if (count_test == 100) {
            eos_task_suspend("sm_led");
        }
        if (count_test == 200) {
            eos_task_resume("sm_led");
        }
        eos_delay_ms(100);
    }
}

int main(void)
{
    if (SysTick_Config(SystemCoreClock / 1000) != 0)
        while (1);
    
    eos_init();                                     // EventOS初始化

#if (EOS_USE_SM_MODE != 0)
    eos_sm_led_init();                              // LED状态机初始化
#endif
    eos_reactor_led_init();
    
    eos_task_start( &task,
                    "task_test", task_func_test, 3,
                    stack_task, sizeof(stack_task));

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
