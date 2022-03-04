/* include ------------------------------------------------------------------ */
#include "eventos.h"                                // EventOS Nano头文件
#include "event_def.h"                              // 事件主题的枚举
#include "eos_led.h"                                // LED灯闪烁状态机

/* define ------------------------------------------------------------------- */
#if (EOS_USE_PUB_SUB != 0)
static eos_u32_t eos_sub_table[Event_Max];          // 订阅表数据空间
#endif

/* main function ------------------------------------------------------------ */
int main(void)
{
    eos_init();                                     // EventOS初始化
#if (EOS_USE_PUB_SUB != 0)
    eos_sub_init(eos_sub_table, Event_Max);         // 订阅表初始化
#endif

    eos_led_init();                                 // LED状态机初始化

    eos_run();                                      // EventOS启动

    return 0;
}