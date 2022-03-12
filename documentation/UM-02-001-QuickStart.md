# 快速入门
------
返回[README文档](../README.md)。

#### 一、简介
**EventOS Nano**，是一个面向单片机、事件驱动的嵌入式开发平台。它主要有两大技术特色：一是事件驱动，二是超轻量。**EventOS Nano**以及其母项目**EventOS**，目标是开发一个企业级的嵌入式开发平台，以**事件总线**为核心，打造一个统一的嵌入式技术生态，为广大企业用户和嵌入式开发者们，提供搞可靠性的、高性能的、现代且高开发效率的嵌入式开发环境。具体详述见[README文档](../README.md)。

**EventOS Nano**，是一个事件驱动的嵌入式编程平台。关于什么是事件，请参考博客文档[如何理解事件](../blog/%E5%A6%82%E4%BD%95%E7%90%86%E8%A7%A3%E4%BA%8B%E4%BB%B6.md)。

#### 二、快速入门
**EventOS Nano**的入门非常简单。除源码外，只需要实现三个代码，就可以使用EventOS Nano来编写程序。
+ **main.c** main函数，初始化和启动**EventOS Nano**。
+ **eos_port.c** 如EventOS在特定平台上的接口实现，也就是EventOS Nano移植的相关代码。
+ **eos_led.c** LED的闪烁状态机。LED灯闪烁，就是单片机界的Hello World。相信是很多人的入门代码。

下面就每一个文件的实现进行详细说明。

1. **main.c**
从EventOS启动的过程非常简单，短短几个步骤就能启动。
``` C
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
    // EventOS Nano的初始化
    eos_init();                                     // EventOS初始化
    eos_sub_init(eos_sub_table);                    // 订阅表初始化
    eos_event_pool_init(eos_heap_memory, 1024);     // 事件池初始化

    // 状态机模块的初始化
    eos_led_init();                                 // LED状态机初始化

    // 启动EventOS Nano。
    eos_run();                                      // EventOS启动并运行

    return 0;
}
```
这里有一个头文件，必须重点说明一下，那就是**event_def.h**。在QP和EventOS Nano中，事件主题的定义是使用枚举来定义的。但反观很多事件驱动和消息驱动的应用，如**ROS**和**MQTT**的实现，其事件（消息）的定义，是使用字符串的。二者各有利弊，枚举的好处是节省RAM和ROM，适合资源受限的场合，字符串的好处是，使用方便，不产生耦合，利于实现分布式。要知道，EventOS Nano和QP中所使用的Actor模型，是天生具备分布式能力，但枚举定义的事件主题，让这个分布式能力大打折扣。也就是说，在一个联网的分布式系统，每一个节点上必须有一个完全相同的**event_def.h**，消息才能无障碍的在节点间传输，这显然是不现实的。因此，在**EventOS**中，消息的主题就采用了字符串进行定义，这样**EventOS**可以毫无障碍的支持分布式特性和跨平台开发。

但在**EventOS Nano**中，依然选择了使用枚举在定义事件主题，这个由**EventOS Nano**的定位所决定的。**EventOS Nano**就是面向资源受限的单片机的。而后续，**Event Bridge**机制将被引入**EventOS Nano**，它会将两种事件主题进行互转，从而以一种间接方式在**EventOS**节点和**EventOS Nano**间建立分布式连接。

事件主题**event_def.h**的定义如下。
``` C
#include "eventos.h"

enum {
    Event_Test = Event_User,                // 事件主题的定义从Event_User开始，小于Event_User的是系统事件。
    Event_Time_500ms,

    Event_Max
};
```
2. **eos_port.c**
移植文件，在《UM-02-002 EventOS Nano移植文档》中已经详细说明，不再赘述。

3. **eos_led.c**和**eos_led.h**
头文件不说了，重点说.c文件，也就是状态机是如何使用的。
``` C
/* include ------------------------------------------------------------------ */
#include "eos_led.h"                    // 模块头文件
#include "eventos.h"                    // EventOS头文件
#include "event_def.h"                  // 事件定义头文件
#include <stdio.h>                      // 标准输入输出库

/* data structure ----------------------------------------------------------- */
typedef struct eos_led_tag {            // LED类
    eos_sm_t super;

    eos_bool_t status;
} eos_led_t;

static eos_led_t led;                   // led对象，单例模式

/* static state function ---------------------------------------------------- */
// 初始状态
static eos_ret_t state_init(eos_led_t * const me, eos_event_t const * const e);
// Led的ON状态
static eos_ret_t state_on(eos_led_t * const me, eos_event_t const * const e);
// Led的Off状态
static eos_ret_t state_off(eos_led_t * const me, eos_event_t const * const e);

/* api ---------------------------------------------------- */
void eos_led_init(void)
{
    static eos_u32_t queue[32];                 // 事件队列
    eos_sm_init(&led.super, 1, queue, 32);      // 状态机初始化
                                                // 状态机启动，以state_init作为初始状态。
    eos_sm_start(&led.super, EOS_STATE_CAST(state_init));

    led.status = 0;
}

/* static state function ---------------------------------------------------- */
static eos_ret_t state_init(eos_led_t * const me, eos_event_t const * const e)
{
    // 订阅事件Event_Time_500ms
    EOS_EVENT_SUB(Event_Time_500ms);
    // 使事件Event_Time_500ms，每隔500ms就被发送一次。
    eos_event_pub_period(Event_Time_500ms, 500);

    return EOS_TRAN(state_off);
}

static eos_ret_t state_on(eos_led_t * const me, eos_event_t const * const e)
{
    switch (e->topic) {
        case Event_Enter:                           // 状态state_on的进入事件
            printf("State On!\n");
            me->status = 1;
            return EOS_Ret_Handled;

        case Event_Time_500ms:                      // 收到Event_Time_500ms，跳转到state_off
            return EOS_TRAN(state_off);

        default:
            return EOS_SUPER(eos_state_top);
    }
}

static eos_ret_t state_off(eos_led_t * const me, eos_event_t const * const e)
{
    switch (e->topic) {
        case Event_Enter:                           // 状态state_on的进入事件
            printf("State Off!\n");
            me->status = 0;
            return EOS_Ret_Handled;

        case Event_Time_500ms:                      // 收到Event_Time_500ms，跳转到state_on
            return EOS_TRAN(state_on);

        default:
            return EOS_SUPER(eos_state_top);
    }
}
```

