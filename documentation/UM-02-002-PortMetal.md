# EventOS Nano移植文档（裸机简易版）
-------
**EventOS Nano**在单片机裸机上的移植，目前有三个接口函数和三个回调函数需要实现。这些函数实现后，就能对**EventOS Nano**在单片机上进行测试。

### 接口函数
这些接口函数，都是**EventOS Nano**对硬件或者平台产生依赖的地方。

#### 1. **时间接口函数**

获取当前的时间基准。在ARM Cortex-M系列单片机，可以参考以下的实现。注意**EventOS Nano**使用的时间，是以毫秒为单位。因此定时器中断时间单位，决定了系统时间递增的单位。

首先在**eventos_config.h**文件中设置一个宏。这个宏的大小，就是时间基本的毫秒数。
``` C
#define EOS_TICK_MS                             1
```

然后调用eos_tick()函数，如下所示。
``` C
#include "eventos.h"
// 1ms中断
void SysTickHandler(void)                       // 1ms中断
{
    eos_tick();
}
```

#### 2. **进入临界区接口**与**退出临界区接口**

这里直接关闭或者打开全局中断即可。在ARM Cortex-M系列单片机，可以参考以下的实现。

``` C
void eos_port_critical_enter(void)
{
    __disable_irq();                            // 当IDE为MDK时
}

void eos_port_critical_exit(void)
{
    __enable_irq();                            // 当IDE为MDK时
}
```

#### 3.  **断言接口**

**EventOS Nano**推崇防御式编程，内部运用了大量的断言检查程序运行的合法性，以便**EventOS Nano**自我发现一些错误，如：程序BUG、用户的错误使用、错误的函数参数、内存泄漏、数据越界、野指针等问题。因此，断言接口要精心设计和实现。在ARM Cortex-M系列单片机裸机上，可以参考以下的实现。如果使用了RTOS，实现起来可能会有所不同，后续的文档中再讨论。

``` C
void eos_port_assert(eos_u32_t error_id)
{
    // 断言信息的打印。
    // 注意user_print时用户自己实现的打印函数，可以是串口、RTT或者其他打印方式。
    user_print("ASSERT >>> Module: EventOS Nano, ErrorId: %d.\n", error_id);

    // 进入无限循环
    while (1) {
        /* User code */
    }
}
```

### 回调函数
**EventOS Nano**有三个回调函数需要用户实现，这样程序才能编译通过。

#### 1. **空闲回调函数**
当**EventOS Nano**没有任何事件需要处理的时候，就会调用此函数。在这里可能需要实现的功能有：硬件设备的轮询、随机数的计算等。如果没有，实现一个空函数即可。
``` C
void eos_hook_idle(void)
{
}
```

#### 2. **启动回调函数**
在**EventOS Nano**初始化完毕，且状态机运行之前，会调用这个回调函数。这里可以对硬件设备进行初始化、对功能模块或者中间件进行初始化等。当然，这些初始化函数，在main函数或者其他位置实现也是没问题的。

``` C
void eos_hook_start(void)
{
    // 硬件初始化
    /* User code */

    // 功能模块初始化
    /* User code */
}
```

#### 3. **停止回调函数**
**EventOS Nano**主张**防御式编程**，也就是说，需要用大量断言去检查程序运行的过程，

在**EventOS Nano**进入断言后，或者用户调用eventos_stop函数将EventOS停止后，此回调函数会被调用。这个函数主要用于有序关闭敏感设备（关闭电机运行），或者向外界提供信息（打开报警器，显示报警界面，或者打印报警信息等）。

``` C
void eos_hook_stop(void)
{
    // 有序关闭敏感设备
    motor_stop(Motor_Left);
    motor_stop(Motor_Right);

    // 显示必要信息
    led_set_status(LedStatus_Stop);
}
```
