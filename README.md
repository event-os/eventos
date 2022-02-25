# EventOS Nano源码说明
-------
### EventOS Nano是什么？
EventOS Nano，是一个面向小资源单片机（Cortex-M0/16/8位单片机）、事件驱动的嵌入式开发平台。它主要有两大技术特色：一是事件驱动，二是超轻量。

总体而言，EventOS Nano的基本思想，脱胎于[QP状态机框架](www.state-machine.com)。但与QP框架相比，EventOS Nano项目做了很多理念上和实现上的调整。这些调整包含但不限于：
+ **事件总线**为核心组件，灵活易用，是进行线程（状态机）间同步或者通信的主要手段，也是对EventOS分布式特性和跨平台开发进行支持的唯一手段。可以作为子系统，“悄悄”嵌入到其他软件系统中去。
+ **极度轻量，便于嵌入式**，除事件总线外的所有特性（层次状态机、平面状态机、发布-订阅机制、事件携带数据、事件桥等）均可裁剪，将资源占用降至极限。
+ 仅支持**软实时**，不再关注硬实时特性，将对资源（尤其是RAM）的占用降至极限。
+ 事件支持广播发送，或者发布-订阅机制两种方式，不再支持QP所支持事件的直接发送方式。
+ 以时间事件的形式，优雅实现了软定时器。
+ 事件与时间事件的实现与QP有很大不同，使用更加简单。
+ API的设计，更加符合本土嵌入式工程师的习惯。
+ 更加便于移植，只需实现少数几个接口函数即可。
+ 暂时不考虑支持QM工具（如果网友强烈要求，可以再讨论）。
+ 未来会使用**Event Bridge**机制与EventOS打通事件总线，以便对EventOS的分布式特性进行支持。
+ 重点关注三种应用场景：小资源单片机，作为模块向其他软件系统的嵌入和可靠性要求较高的嵌入式场
如果你想写一个清晰的、结构健壮且利于维护的源代码，又不想使用传统的RTOS，那么EventOS Nano是你的不二选择。

### 为什么叫做EventOS Nano？
之所以叫**Nano**，是因为它真的非常轻量，EventOS Nano的设计目标是，经过适当裁剪，能够在MCS51（ROM：4KByte，RAM：256Byte）中良好运行。
之所以叫**EventOS Nano**，一是因为它的技术特色与核心思想，就是**事件驱动**；二是因为它是另外一个开源项目**EventOS**（准备中）的简化实现，**EventOS**是一个**事件驱动**的、**分布式**的、**可跨平台开发**的嵌入式RTOS，面向32位单片机和更高处理器的嵌入式开发平台。

### 代码结构
+ **核心代码**
    + **eventos/eventos.c** EventOS Nano状态机框架的实现
    + **eventos/eventos.h** 头文件
    + **eventos/eventos_config.h** 对EventOS Nano进行配置与裁剪
+ **第三方代码库**
    + **RTT** Segger JLink所提供的日志库，依赖于JLink硬件。
    + **unity** 单元测试框架
+ **例程代码**
    + **freertos** 对FreeRTOS的适配例程（未完成）。
    + **posix** 对符合POSIX标准的操作系统（如Linux、VxWork、MinGW等)的适配例程（未完成）。
    + **arm-cm0** 对ARM Cortex-M0芯片的裸机运行（无RTOS）的例程（未完成）。
    + **arm-cm3** 对ARM Cortex-M3芯片的裸机运行（无RTOS）的例程（未完成）。
    + **test** 对源码进行的单元测试例程。
    + **digital_watch** 电子表例程，状态机的典型应用。
+ **tools**
一些脚本和工具。
+ **文档**
文档包含Doxygen代码文档的生成路径（未完成）、图片、代码相关文档（如快速入门文档、移植文档、开发环境搭建说明文档等）。

### 快速入门
EventOS Nano的移植和入门，在**documentation**文件夹里，可以参考以下几个文档：
+ [快速入门文档](/documentation/UM-02-001-QuickStart.md)
+ [裸机移植文档](/documentation/UM-02-002-PortMetal.md)
+ [开发环境搭建](/documentation/UM-02-003-DevEnv.md)

### 未来的开发路线
**EventOS Nano**曾经让我在过去的工作中受益匪浅，让我非常高效的写出了很多可靠的程序，能力和回报都有了质的提升。未来，**EventOS Nano**这个项目我会一直完善下去。我的目标是，在2022年，将**EventOS Nano**项目做成Gitee上的GVP项目，造福更多的嵌入式工程师。但目前已经完成的工作还远远不足，就我识别到的，将来需要完善的工作列举如下：
1. 良好的注释和文档
    + 【完成】UM-001 快速入门文档
    + 【完成裸机】UM-002 移植文档（含裸机和RTOS上的移植）
    + 【完成】UM-003 开发环境搭建说明
1. 【完成】将所有的变量和API以EventOS风格进行重构
1. 将C99标准实现的代码，修改为Clean C来实现。
1. 【完成】将构建工具修改为SCons
1. 【完成】增加内存管理功能，以支持任意长度的事件参数，修改掉原来的事件机制，优化RAM占用。
1. 【完成】将mdebug库删除，只保留断言接口，如果有日志必要，更换为elog库。（elog库，原名mlog，是我过去一直使用的日志库，整理后也会开源）
1. 【完成】将单元测试代码按照Unity框架进行重构
1. 严谨而完整的单元测试。
1. 【完成】对FSM模式的支持，以适用于最小资源的单片机ROM
1. 将每个状态机都有一个事件队列，修改为整个程序只有一个事件队列，通过链表实现事件池即队列。借鉴Nordic事件订阅的方式。
1. 每一个功能模块的裁剪。
    + 【完成】SM模式
    + 【完成】HSM模式
    + 【完成】SUB机制
    + Time Event
    + Event Data
1. 增加Doxygen风格的注释，并生成文档。
1. 对EventOS的eBridge（事件桥接）功能
1. 对ARM Cortex-M0 M3 M4 M7等单片机上的移植，增加对最常见型号单片机的支持，如STM32F103等。
    + ARM Cortex-M0
    + ARM Cortex-M3
    + ARM Cortex-M4
    + ARM Cortex-M7
    + 【完成】POSIX
    + FreeRTOS
    + 【完成】Test
    + Hello
    + Digital Watch（POSIX版）
    + Digital Watch（RTT版）
1. 对常见的IDE的支持
1. 对常见的RTOS的支持
1. 增加对RISC-V内核的支持
1. 网友的反馈与要求

### 联系方式
邮箱：event-os@outlook.com

除了邮箱之外，也可以加微信联系我，请注明**技术讨论**等相关字样。

![avatar](/documentation/figures/wechat.jpg)