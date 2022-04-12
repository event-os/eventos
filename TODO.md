# EventOS Nano需要进行的优化。
---------
+ 添加返回历史状态的功能
+ 整理eos.enabled的用法，使之更加简洁。
+ actor_exist和enabled可以合为一体。
+ 【完成】将Queue功能与Heap进行隔绝。
+ Doxgen风格的注释
+ 对EventOS的eBridge（事件桥接）功能
+ 对ARM Cortex-M0 M3 M4 M7等单片机上的移植，增加对最常见型号单片机的支持，如STM32F103等。
    + 【完成】ARM Cortex-M0
    + 【完成】ARM Cortex-M3
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
1. 修复掉Copy Tool中，Copy区里不能含有中文字符的BUG。
1. M0和M3的例程进行实物测试。
1. Posix例程中需要处理时间溢出问题。
1. 对事件携带数据进行单元测试，对事件数据的长度，进行单元测试。
1. 分别对M0和其他平台进行4字节对齐和非字节对齐的处理。
1. 增加了不携带数据的事件的实现，不使用HEAP。
1. 重新增加对MAGIC的校验，可以使用宏来关闭。
1. Config文件，使用MDK支持的格式。
1. malloc申请时，用尽了所有的内存的情况
