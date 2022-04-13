/* include ------------------------------------------------------------------ */
#include "stm32h7xx.h"
#include "eventos.h"                                // EventOS Nano头文件
#include "eos_led.h"                                // LED灯闪烁状态机

/* main function ------------------------------------------------------------ */
uint8_t stack_task[1024];
eos_task_t task;
uint8_t stack_task_event[1024];
eos_task_t task_event;
uint8_t stack_task_e_specific[1024];
eos_task_t task_e_specific;
uint32_t count_test = 0;

void task_func_test(void *parameter)
{
    (void)parameter;
    
    while (1) {
        count_test ++;
        eos_event_send_topic("task_event", "Event_One");
//        eos_event_send_topic("task_e_specific", "Event_One");
//        if ((count_test % 10) == 0) {
//            eos_event_send_topic("task_e_specific", "Event_Specific");
//        }
//        if (count_test == 100) {
//            eos_task_suspend("sm_led");
//        }
//        if (count_test == 200) {
//            eos_task_resume("sm_led");
//        }
        eos_delay_ms(100);
    }
}

uint32_t event_one_count = 0;
uint32_t task_event_error = 0;
void task_func_event_test(void *parameter)
{
    (void)parameter;
    
    while (1) {
        eos_event_t e;
        if (eos_task_wait_event(&e, 10000) == false) {
            task_event_error = 1;
            continue;
        }

        if (eos_event_topic(&e, "Event_One")) {
            event_one_count ++;
        }
    }
}

uint32_t e_one_count = 0;
uint32_t e_specific_count= 0;
uint32_t task_e_error = 0;
void task_func_e_specific_test(void *parameter)
{
    (void)parameter;
    
    while (1) {
        eos_event_t e;
        if (eos_task_wait_specific_event(&e, "Event_Specific", 10000)) {
            task_event_error = 1;
            continue;
        }

        if (eos_event_topic(&e, "Event_One")) {
            e_one_count ++;
        }
        
        if (eos_event_topic(&e, "Event_Specific")) {
            e_specific_count ++;
        }
    }
}


int main(void)
{
    if (SysTick_Config(SystemCoreClock / 1000) != 0)
        while (1);
    
    eos_init();                                     // EventOS初始化

//#if (EOS_USE_SM_MODE != 0)
//    eos_sm_led_init();                              // LED状态机初始化
//#endif
//    eos_reactor_led_init();
    
    eos_task_start( &task,
                    "task_test", task_func_test, 1,
                    stack_task, sizeof(stack_task));
    eos_task_start( &task_event,
                    "task_event", task_func_event_test, 3,
                    stack_task_event, sizeof(stack_task_event));
//    eos_task_start( &task_e_specific,
//                    "task_e_specific", task_func_e_specific_test, 2,
//                    stack_task_e_specific, sizeof(stack_task_e_specific));
    
    eos_run();                                      // EventOS启动

    return 0;
}

uint32_t count_tick = 0;
void SysTick_Handler(void)
{
    //count_tick ++;
    eos_tick();
}

void HardFault_Handler(void)
{
    while (1) {
    }
}
