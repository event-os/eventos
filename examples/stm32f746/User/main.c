/* include ------------------------------------------------------------------ */
#include "stm32f7xx.h"
#include "eventos.h"                                // EventOS Nano头文件
#include "eos_led.h"                                // LED灯闪烁状态机

/* main function ------------------------------------------------------------ */
uint64_t stack_task[512];
eos_task_t task_test;
uint64_t stack_task_event[512];
eos_task_t task_event;
uint64_t stack_task_e_specific[512];
eos_task_t task_e_specific;
uint32_t count_test = 0;

typedef struct e_value {
    uint32_t count;
    uint32_t value;
} e_value_t;

void block_delay(uint32_t ms)
{
    while (ms --) {
        uint32_t temp = 8500;
        while (temp --) {
            __nop();
        }
    }
}


void task_func_test(void *parameter)
{
    (void)parameter;
    
    while (1) {
        count_test ++;
        eos_event_send_topic("task_event", "Event_One");
        eos_event_send_topic("task_e_specific", "Event_Two");
        
        e_value_t e_value;
        e_value.count = count_test;
        e_value.value = 12345678;
        eos_event_send_value("task_event", "Event_Value", &e_value);
        
        if ((count_test % 10) == 0) {
            eos_event_send_topic("task_event", "Event_Specific");
            eos_event_send_topic("task_e_specific", "Event_Two");
        }
        if (count_test == 100) {
            eos_task_suspend("sm_led");
        }
        if (count_test == 200) {
            eos_task_resume("sm_led");
        }
        block_delay(10);
        eos_delay_ms(100);
    }
}

uint32_t event_one_count = 0;
uint32_t event_specific_count = 0;
uint32_t task_event_error = 0;
uint32_t event_value_count = 0;
e_value_t e_value_recv;
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
        
        if (eos_event_topic(&e, "Event_Specific")) {
            event_specific_count ++;
        }
        
        // TODO BUG。值事件收不到正确的值，也不能得到正确的次数。
        if (eos_event_value_recieve(&e, "Event_Value", &e_value_recv)) {
            event_value_count ++;
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

        if (eos_event_topic(&e, "Event_Two")) {
            e_one_count ++;
        }
        
        if (eos_event_topic(&e, "Event_Specific")) {
            e_specific_count ++;
        }
    }
}

e_value_t e_value;

int main(void)
{
    SystemCoreClockUpdate();

    SCB_EnableICache(); /* Enable I-Cache */
    SCB_EnableDCache(); /* Enable D-Cache */
    
    if (SysTick_Config(SystemCoreClock / 1000) != 0)
        while (1);
    
    eos_init();                                     // EventOS初始化
    
    eos_event_attribute_value("Event_Value", &e_value, sizeof(e_value_t));

//#if (EOS_USE_SM_MODE != 0)
    eos_sm_led_init();                              // LED状态机初始化
//#endif
    eos_reactor_led_init();
    
    eos_task_start( &task_test,
                    "task_test", task_func_test, 1,
                    stack_task, sizeof(stack_task));
    eos_task_start( &task_event,
                    "task_event", task_func_event_test, 2,
                    stack_task_event, sizeof(stack_task_event));
    eos_task_start( &task_e_specific,
                    "task_e_specific", task_func_e_specific_test, 3,
                    stack_task_e_specific, sizeof(stack_task_e_specific));
    
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
