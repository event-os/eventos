/* include ------------------------------------------------------------------ */
#include "stm32f4xx.h"
#include "eventos.h"                                // EventOS Nano头文件
#include "eos_led.h"                                // LED灯闪烁状态机
#include "system.h"
#include "esh.h"
#include <string.h>

/* main function ------------------------------------------------------------ */
uint64_t stack_task[64];
eos_task_t task_test;
uint64_t stack_task_event[64];
eos_task_t task_event;
uint64_t stack_task_e_specific[64];
eos_task_t task_e_specific;

uint32_t count_test[32];

typedef struct e_value
{
    uint32_t count;
    uint32_t value;
} e_value_t;

void block_delay(uint32_t ms)
{
    while (ms --)
    {
        uint32_t temp = 8500;
        while (temp --)
        {
            __nop();
        }
    }
}

char buffer[32] = {0};
uint32_t count = 0;
void task_func_test(void *parameter)
{
    (void)parameter;
    memset(count_test, 0, sizeof(count_test));
    
    while (1)
    {
        uint32_t i = 0;
        
        count_test[i ++] ++;
        if (count_test[0] > 100)
        {
            count_test[i] = count_test[i];
        }
        
        eos_event_send("task_event", "Event_One");
        count_test[i ++] ++;
        
        eos_event_send("task_e_specific", "Event_Two");
        count_test[i ++] ++;
        
        e_value_t e_value;
        e_value.count = count_test[0];
        e_value.value = 12345678;
        eos_db_block_write("Event_Value", &e_value);
        count_test[i ++] ++;
        
        eos_event_send("task_event", "Event_Value");
        count_test[i ++] ++;
        
        eos_db_block_write("Event_Value", &e_value);
        count_test[i ++] ++;
        
        eos_event_send("task_event", "Event_Value");
        count_test[i ++] ++;
        
        eos_db_block_write("Event_Value_Link", &e_value);
        count_test[i ++] ++;
        
        eos_event_send("task_event", "Event_Value_Link");
        count_test[i ++] ++;
        
        if ((count_test[0] % 10) == 0)
        {
            eos_event_send("task_event", "Event_Specific");
            count_test[i ++] ++;
            
            eos_event_send("task_e_specific", "Event_Two");
            count_test[i ++] ++;
        }
//        if (count_test == 100) {
//            eos_task_suspend("sm_led");
//        }
//        if (count_test == 200) {
//            eos_task_resume("sm_led");
//        }
        
//        esh_log("-------------------------\n");
//        
        eos_delay_ms(100);
    }
}

uint32_t event_one_count = 0;
uint32_t event_specific_count = 0;
uint32_t task_event_error = 0;
uint32_t event_value_count = 0;
uint32_t event_value_link_count = 0;
e_value_t e_value_recv;
e_value_t e_value_link_recv;

void task_func_event_test(void *parameter)
{
    (void)parameter;
    
    eos_event_sub("Event_Value_Link");
    
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
        
        if (eos_event_topic(&e, "Event_Value")) {
            eos_db_block_read("Event_Value", &e_value_recv);
            event_value_count ++;
        }
        
        if (eos_event_topic(&e, "Event_Value_Link")) {
            eos_db_block_read("Event_Value_Link", &e_value_link_recv);
            event_value_link_count ++;
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
        if (eos_task_wait_specific_event(&e, "Event_Specific", 10000))
        {
            task_event_error = 1;
            continue;
        }

        if (eos_event_topic(&e, "Event_Two"))
        {
            e_one_count ++;
        }
        
        if (eos_event_topic(&e, "Event_Specific")) {
            e_specific_count ++;
        }
    }
}

uint8_t db_memory[512];
uint32_t count_tick = 0;

elog_device_t dev_esh;

void esh_flush(void)
{
}

EOS_TAG("main")

int main(void)
{
    SystemCoreClockUpdate();
    
    if (SysTick_Config(SystemCoreClock / 1000) != 0)
        while (1);

    dev_esh.enable = 1;
    dev_esh.level = eLogLevel_Debug;
    dev_esh.name = "eLogDev_Esh";
    dev_esh.out = esh_log;
    dev_esh.flush = esh_flush;
    dev_esh.ready = esh_ready;
    
    elog_init();
    elog_set_level(eLogLevel_Debug);
    esh_init();
    esh_start();
    count_tick ++;
    
    elog_device_register(&dev_esh);
    elog_start();
    
    eos_init();                                     // EventOS初始化
    
    eos_db_init(db_memory, 512);
    
    eos_db_register("Event_Value", sizeof(e_value_t), EOS_DB_ATTRIBUTE_VALUE);
    eos_db_register("Event_Value_Link", sizeof(e_value_t),
                    (EOS_DB_ATTRIBUTE_VALUE | EOS_DB_ATTRIBUTE_LINK_EVENT));
    
    eos_sm_led_init();                              // LED状态机初始化
    eos_reactor_led_init();
    
    eos_task_start( &task_test,
                    "task_test", task_func_test, TaskPriority_Test,
                    stack_task, sizeof(stack_task));
    eos_task_start( &task_event,
                    "task_event", task_func_event_test, TaskPriority_Event,
                    stack_task_event, sizeof(stack_task_event));
    eos_task_start( &task_e_specific,
                    "task_e_specific", task_func_e_specific_test, TaskPriority_Event_Specific,
                    stack_task_e_specific, sizeof(stack_task_e_specific));
    
    eos_run();                                      // EventOS启动

    return 0;
}


uint32_t systick = 0;
uint32_t systick2 = 0;

void SysTick_Handler(void)
{
    if (critical_count > 0) {
        systick2 ++;
    }
    count_tick ++;
    eos_interrupt_enter();
    eos_tick();
    eos_interrupt_exit();
}

void HardFault_Handler(void)
{
    while (1) {
    }
}