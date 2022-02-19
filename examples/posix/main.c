/* include ------------------------------------------------------------------ */
#include "eventos.h"                                // EventOS Nano头文件
#include "event_def.h"                              // 事件主题的枚举
#include "eos_led.h"                                // LED灯闪烁状态机

/* define ------------------------------------------------------------------- */
static eos_u32_t eos_sub_table[Event_Max];          // 订阅表数据空间
static eos_u8_t eos_heap_memory[1024];              // 事件池空间

/* main function ------------------------------------------------------------ */
int main(void)
{
    eventos_init();                                 // EventOS初始化
    eos_sub_init(eos_sub_table);                    // 订阅表初始化
    eos_event_pool_init(eos_heap_memory, 1024);     // 事件池初始化

    eos_led_init();                                 // LED状态机初始化

    eventos_run();                                  // EventOS启动

    return 0;
}