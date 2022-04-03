### 如何理解EventOS Nano中的事件？

**EventOS Nano**项目地址，在[Gitee EventOS Nano](https://gitee.com/event-os/eventos-nano.git)处。

------

事件机制，是**EventOS**的核心内容，是整个**EventOS**大厦的基础。那么，有必要了解一下什么是事件，才能正确的使用事件，编写合格的事件驱动的程序。很多嵌入式的小伙伴们，对事件机制一头雾水。但了解过事件机制的人，却对其非常推崇，因为借助于事件机制，我们可以写出极高性能和可靠性的程序。

### 一、什么是事件？

个人认为，在嵌入式系统中，事件可以这么定义：

**嵌入式系统中，一切在发生或者已发生的事情，只要导致了内部数据的变化，都可以被认为是事件。**

比如，按键的按下或者抬起，串口接收到一组数据，CAN总线接收到一个报文，某个通信协议连接失败，定时500ms时间到，电池电量将到某个阈值之下，温度传感器高于某个阈值，避障传感器检测到障碍物，等等等等，所有这些，都可以认为是事件。

事件，说到底，是对系统中所发生的事，进行的高级抽象。**代码，是对物理世界的抽象；事件，是对物理世界所发生的事的抽象。**

运用事件机制，可以有效地对RTOS中存在的多种IPC（线程间通信）方式，进行整合。

### 二、EventOS Nano中的事件机制

**EventOS Nano**中的事件机制，与RTOS中的事件标志组的概念不甚相同。因此，不能拿RTOS中的事件，硬套在**EventOS Nano**中。**EventOS Nano**中的事件，可以理解为消息。

在**EventOS**中，一个事件由几个元素构成：事件主题、数据长度和事件携带的数据。我们在**EventOS Nano**中，对事件所抽象的数据结构如下所示。

``` C
typedef struct eos_event {
    eos_topic_t     topic;
    void *          data;
    eos_u16_t       size;
} eos_event_t;
```

这和我们日常生活的体验，是一致的。比如，娱乐圈里发生了一个大新闻，某台湾女星，又结婚了。你会怎么向同事们描述这件事？“嘿！知道吗？最近发生一个大新闻（事件主题），大爱思又结婚了，跟一个韩国大叔！（事件携带的数据）”翻译成代码，就是下面这样的：
``` C
// 大爱思又结婚了
const char *news = "S has been married again with one Bangzi!"
eos_event_pub(Event_BigNews, (void *)news, strlen(news));
```
或者，也可以这么说：“嘿！知道吗？最近大爱思又结婚了（事件主题），跟一个韩国大叔！（事件携带的数据）”翻译成代码，就是下面这样的：
``` C
// 大爱思又结婚了
const char *news = "With one Bangzi called JuJunye!"
eos_event_pub(Event_S_Married, (void *)news, strlen(news));
```
（感谢他们夫妻二位提供素材！祝新婚快乐！）

事件无时无刻不在发生，但我们只关注和处理那些有用的事件。那么什么是有用的事件呢？比如某个机器人的电池电压，在使用中，一直在缓慢降低，时时刻刻都会产生事件，但我们仅仅关注两种事件即可，一个每隔10S（举例而已）的电压值，用于计算电池电量，另外一个是电池电量低于某个阈值（用于提醒机器人去充电）。

针对这样的数据结构，我们列举了几个事件组成。

|  事件源   | 主题 | 长度 | 数据 |
| --- | --- | --- | --- |
| 按键按下或抬起 | Event_Button | 1 | 状态值 |
| 串口接收数据 | Event_UartRecieve | N | 数据 |
| 设备急停 | Event_EmgStop | 0 | 无 |
| 电池电量低 | Event_LowPower | 2 | 2750mv |

### 三、事件是如何发送的？

**EventOS Nano**支持**发布订阅**模式，或者广播方式，来实现事件的发送和接收。这两种事件发送方式，都有一个好处，就是能够完全的解耦发送方和接收方。发送方，只需要把事件发布出去，而不必管有没有人接收这个事件，也不必管谁接收这个事件；而事件接收者，只需要订阅某个事件（广播模式下连订阅都不需要），而不必管谁发布了这个事件。这种方式，是对软件模块间的彻底解耦，不仅能对各个模块间进行解耦，还能对应用层和底层进行解耦（底层把收到的数据，以事件的形式发送到应用层）。

打开和关闭发布-订阅模式，是在**eventos_config.h**中的宏**EOS_USE_PUB_SUB**设置为1或者0（1为打开）。

根据**EventOS Nano**的API，在发送事件时的代码如下所示。

``` C
// 按下按下或者抬起
bool status_button = true;
eos_event_pub(Event_Button, (void *)&status_button, sizeof(bool));
// 串口接收数据
eos_u32_t uart_count_recv = 128;
eos_event_pub(Event_UartRecieve, (void *)buffer_uart_recieve, uart_count_recv);
// 设备急停
eos_event_pub_topic(Event_EmgStop);
// 电池电量低
eos_u16_t battery_voltage = 2750;               // mv
eos_event_pub(Event_LowPower, (void *)&battery_voltage, sizeof(eos_u16_t));
```

这样就很容易把事件及其携带的数据发送了出去。剩下的一切，都交给了框架。

### 四、事件是如何接收的？
前面说了，**EventOS Nano**事件机制，是支持**发布订阅**模式，或者广播方式。在广播模式下，不必做任何事情，事件就会发送到Actor上；在发布-订阅模式下，也只需要订阅事件。对于各个软件模块来说，**EventOS Nano**相当于一个事件的中转站，它有如下几个功能：
+ 将Actor和底层发布出来的事件及其携带的数据，暂存在事件队列；
+ 根据各个Actor对事件的订阅情况，和各个Actor的优先级，调用Actor的事件处理函数（Reactor模式）或者状态函数（状态机模式）。
+ 管理和回收事件缓冲区。

订阅事件的代码如下：
``` C
EOS_EVENT_SUB(Event_Time_500ms);                
```

在广播模式下，每一个事件，都会被发送到每一个Actor。如果当前Actor并不需要某个事件，就不必写事件处理代码就可以。并不会带来错误，也不会带来任何多余的RAM，只是稍稍浪费CPU而已。

于是，在状态机中，事件处理代码如下：
``` C
// ON状态下，接收到500ms的定时事件，跳转到OFF状态
static eos_ret_t state_on(eos_sm_led_t * const me, eos_event_t const * const e)
{
    switch (e->topic) {
        case Event_Time_500ms:                  // 接收到事件Event_Time_500ms
            return EOS_TRAN(state_off);

        default:                                // 广播模式的多余事件不做处理
            return EOS_SUPER(eos_state_top);
    }
}
```

在Reactor模式下，事件接收代码如下：
``` C
// 每收到1000ms的定时事件，status的值（或者LED状态）会翻转。
static void led_e_handler(eos_reactor_led_t * const me, eos_event_t const * const e)
{
    if (e->topic == Event_Time_1000ms) {
        me->status = (me->status == 0) ? 1 : 0;
    }
    else {
        // 广播模式的多余事件，不做处理。
    }
}
```