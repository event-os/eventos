
/*
 * EventOS V0.2.0
 * Copyright (c) 2021, EventOS Team, <event-os@outlook.com>
 *
 * SPDX-License-Identifier: MIT
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the 'Software'), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS 
 * OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 * IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.event-os.cn
 * https://github.com/event-os/eventos
 * https://gitee.com/event-os/eventos
 * 
 */

#ifndef EVENTOS_H_
#define EVENTOS_H_

#include "eventos_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------------------------------------
EventOS Default Configuration
----------------------------------------------------------------------------- */
#ifndef EOS_MAX_TASKS
#define EOS_MAX_TASKS                           8       // 默认最多8个Actor
#endif

#ifndef EOS_MAX_OBJECTS
#define EOS_MAX_OBJECTS                         256     // 默认最多256个事件
#endif

#ifndef EOS_USE_ASSERT
#define EOS_USE_ASSERT                          1       // 默认打开断言
#endif

#ifndef EOS_USE_SM_MODE
#define EOS_USE_SM_MODE                         0       // 默认关闭状态机
#endif

#ifndef EOS_USE_PUB_SUB
#define EOS_USE_PUB_SUB                         0       // 默认关闭发布-订阅机制
#endif

#ifndef EOS_USE_TIME_EVENT
#define EOS_USE_TIME_EVENT                      0       // 默认关闭时间事件
#endif

#ifndef EOS_USE_EVENT_DATA
#define EOS_USE_EVENT_DATA                      0       // 默认关闭时间事件
#endif

#ifndef EOS_USE_EVENT_BRIDGE
#define EOS_USE_EVENT_BRIDGE                    0       // 默认关闭事件桥
#endif

#include <stdint.h>
#include <stdbool.h>

/* -----------------------------------------------------------------------------
Basic type
----------------------------------------------------------------------------- */
typedef enum eos_bool {
    EOS_False = 0,
    EOS_True = !EOS_False,
} eos_bool_t;

#define EOS_NULL                        ((void *)0)

#if (EOS_TEST_PLATFORM == 32)
typedef uint32_t                        eos_pointer_t;
#else
#include <stdint.h>
typedef uint64_t                        eos_pointer_t;
#endif

/* -----------------------------------------------------------------------------
EventOS
----------------------------------------------------------------------------- */
// EventOS initialization.
void eos_init(void * const event_queue, uint32_t e_queue_size);
// Run EventOS.
void eos_run(void);
// TODO 优化。修改为毫秒和微秒两个获取时间的函数。System time.
uint64_t eos_time(void);
// uint64_t eos_time_ms(void)；
// uint64_t eos_time_us(void);
// System tick function.
void eos_tick(void);
// 关闭中断
void eos_critical_enter(void);
// 开中断
void eos_critical_exit(void);
// TODO 实现。禁止任务切换
void eos_sheduler_lock(void);
// TODO 实现。关闭禁止任务切换
void eos_sheduler_unlock(void);

/* -----------------------------------------------------------------------------
Task
----------------------------------------------------------------------------- */
/*
 * Defines the prototype to which the application task hook function must
 * conform.
 */
typedef void (* eos_func_t)(void *parameter);

/*
 * Definition of the event class.
 */
// TODO 优化。字节对齐，以最大限度降低RAM占用。
// TODO 优化。修改此数据结构，增加一个对应的私有数据接口，对外不开放这么多信息。
typedef struct eos_event {
    const char *topic;                      // Event Topic
    uint32_t type                   : 3;    // Event Type
    uint32_t size                   : 16;   // Event size
} eos_event_t;

/*
 * Definition of the task class.
 */
typedef struct eos_task {
    uint32_t *sp;
    void *stack;
    uint32_t size;
    uint32_t timeout;
    uint32_t stack_size;              /* stack size */
    uint32_t state                  : 3;
    uint32_t priority               : 6;
    uint32_t id                     : 6;
    uint32_t enabled                : 1;
} eos_task_t;

// 启动任务，main函数或者任务函数中调用。
void eos_task_start(eos_task_t * const me,
                    const char *name,
                    eos_func_t func,
                    uint8_t priority,
                    void *stack_addr,
                    uint32_t stack_size);
// 退出当前任务，任务函数中调用。
void eos_task_exit(void);
// 任务内延时，任务函数中调用，不允许在定时器的回调函数调用，不允许在空闲回调函数中调用。
void eos_delay_ms(uint32_t time_ms);
// 延时，屏蔽事件的接收（毫秒级延时，释放CPU控制权），直到延时完毕。
void eos_delay_no_event(uint32_t time_ms);
// 挂起某任务
void eos_task_suspend(const char *task);
// 删除某任务
void eos_task_delete(const char *task);
// 恢复某任务
void eos_task_resume(const char *task);
// 切换任务
void eos_task_yield(void);
// 任务等待某特定事件
bool eos_task_wait_specific_event(  eos_event_t * const e_out,
                                    const char *topic, uint32_t time_ms);
// 任务阻塞式等待事件
bool eos_task_wait_event(eos_event_t * const e_out, uint32_t time_ms);

/* -----------------------------------------------------------------------------
Timer
----------------------------------------------------------------------------- */
/*
 * Definition of the timer class.
 */
typedef struct eos_timer {
    struct eos_timer *next;
    uint32_t time;
    uint32_t time_out;
    eos_func_t callback;
    uint32_t oneshoot               : 1;
    uint32_t running                : 1;
} eos_timer_t;

// 启动软定时器，允许在中断中调用。
// TODO 优化。此处完全不必面向对象。
void eos_timer_start(   eos_timer_t * const me,
                        const char *name,
                        uint32_t time_ms,
                        bool oneshoot,
                        eos_func_t callback);
// 删除软定时器，允许在中断中调用。
void eos_timer_delete(const char *name);
// 暂停软定时器，允许在中断中调用。
void eos_timer_pause(const char *name);
// 继续软定时器，允许在中断中调用。
void eos_timer_continue(const char *name);
// 重启软定时器的定时，允许在中断中调用。
void eos_timer_reset(const char *name);

/* -----------------------------------------------------------------------------
Event
----------------------------------------------------------------------------- */
// 事件的属性设置 -----------------------------------------
// 设置事件为全局事件，可以发送到事件桥与事件域，进而形成跨CPU的事件总线。
void eos_event_attribute_global(const char *topic);
// 设置不可阻塞事件。在延时时，此类事件进入，延时结束，对此类事件进行立即响应。
void eos_event_attribute_unblocked(const char *topic);
// TODO 优化。数据库实现后，删去这个。设置事件为流事件。流事件不会保留原本的发送时序。
void eos_event_attribute_stream(const char *topic,
                                const char *target,
                                void *memory, uint32_t capacity);
// TODO 优化。数据库实现后，删去这个。设置事件为值事件。值事件不会保留原本的发送时序。
void eos_event_attribute_value(const char *topic, void *memory, uint32_t size);

// 事件的直接发送 -----------------------------------------
// TODO 优化。删除topic字样。直接发送主题事件。
// TODO 优化。不允许在中断中调用。
void eos_event_send_topic(const char *task, const char *topic);
// TODO 优化。允许在中断调用。
void eos_event_send_isr(const char *task, const char *topic);
// TODO 优化。数据库实现后，删去这个。直接发送值事件。允许在中断中调用。
void eos_event_send_value(const char *task, const char *topic, void const *data);
// TODO 优化。数据库实现后，删去这个。直接发送流事件。允许在中断中调用。
void eos_event_send_stream(const char *topic, void const *data, uint32_t size);
// TODO 优化。数据库实现后，删去这个。设定事件的值。允许在中断中调用。
void eos_event_set_value(const char *topic, void *data);

// 事件的广播 --------------------------------------------
// TODO 优化。删除topic字样。广播发布某主题事件。
// TODO 优化。不允许在中断中调用。
void eos_event_broadcast_topic(const char *topic);
// TODO 实现。允许在中断调用。
void eos_event_broadcast_isr(const char *topic);
// TODO 优化。数据库实现后，删去这个。广播发布某值事件。允许在中断中调用。
void eos_event_broadcast_value(const char *topic, void const *data);

// 事件的发布 --------------------------------------------
// TODO 优化。数据库实现后，去掉topic字样。发布主题事件。允许在中断中调用。
void eos_event_pub_topic(const char *topic);
// TODO 优化。数据库实现后，删去这个。发布值事件。允许在中断中调用。
void eos_event_pub_value(const char *topic, void *data);
// 延时发布某主题事件。允许在中断中调用。
void eos_event_pub_delay(const char *topic, uint32_t time_delay_ms);
// 周期发布某主题事件。允许在中断中调用。
void eos_event_pub_period(const char *topic, uint32_t time_period_ms);
// 取消某延时或者周期事件的发布。允许在中断中调用。
void eos_event_time_cancel(const char *topic);

// 事件的订阅 --------------------------------------------
// 事件订阅，仅在任务函数、状态函数或者事件回调函数中使用。
void eos_event_sub(const char *topic);
// 事件取消订阅，仅在任务函数、状态函数或者事件回调函数中使用。
void eos_event_unsub(const char *topic);

// 事件的接收 --------------------------------------------
// TODO 优化。主题事件接收。仅在任务函数、状态函数或者事件回调函数中使用。
bool eos_event_topic(eos_event_t *const e, const char *topic);
// TODO 优化。数据库实现后，删去这个。读取值事件的值。仅在任务函数、状态函数或者事件回调函数中使用。
bool eos_event_value_recieve(eos_event_t *const e, void *value);
// TODO 优化。数据库实现后，删去这个。流事件接收。仅在任务函数、状态函数或者事件回调函数中使用。
int32_t eos_event_stream_recieve(eos_event_t *const e, void *buffer, uint32_t size);
// TODO 优化。数据库实现后，删去这个。读取值事件的值。仅在任务函数、状态函数或者事件回调函数中使用。
void eos_event_get_value(const char *topic, void *value);

/* -----------------------------------------------------------------------------
Key-Value Database
----------------------------------------------------------------------------- */
// TODO 增加持久化设备。
void eos_db_add_device_persistence(const char *device);
// TODO 实现。数据库的注册。
void eos_db_register(   const char *key,
                        void * const data, uint32_t size,
                        uint8_t attribute);
// TODO 实现。块数据的读取。
void eos_db_block_read(const char *key, void * const data);
void eos_db_block_read_isr(const char *key, void * const data);
// TODO 实现。块数据的写入。
void eos_db_block_write(const char *key, void * const data, uint8_t attribute);
void eos_db_block_write_isr(const char *key, void * const data, uint8_t attribute);
// TODO 实现。流数据的读取。
int32_t eos_db_stream_read(const char *key, void *const buffer, uint32_t size);
int32_t eos_db_stream_read_isr(const char *key, void *const buffer, uint32_t size);
// TODO 实现。流数据的写入。
int32_t eos_db_stream_write(const char *key, void *const buffer, uint32_t size);
int32_t eos_db_stream_write_isr(const char *key, void *const buffer, uint32_t size);

/* -----------------------------------------------------------------------------
Reactor
----------------------------------------------------------------------------- */
// 事件处理句柄的定义
struct eos_reactor;
typedef void (* eos_event_handler)( struct eos_reactor *const me,
                                    eos_event_t const * const e);

/*
 * Definition of the Reactor class.
 */
typedef struct eos_reactor {
    eos_task_t super;
    eos_event_handler event_handler;
} eos_reactor_t;

void eos_reactor_init(  eos_reactor_t * const me,
                        const char *name,
                        uint8_t priority,
                        void *stack, uint32_t size);
void eos_reactor_start(eos_reactor_t * const me, eos_event_handler event_handler);

#define EOS_HANDLER_CAST(handler)       ((eos_event_handler)(handler))

/* -----------------------------------------------------------------------------
State machine
----------------------------------------------------------------------------- */
/*
 * Definition of the EventOS reture value.
 */
typedef enum eos_ret {
    EOS_Ret_Null = 0,                       // 无效值
    EOS_Ret_Handled,                        // 已处理，不产生跳转
    EOS_Ret_Super,                          // 到超状态
    EOS_Ret_Tran,                           // 跳转
} eos_ret_t;

#if (EOS_USE_SM_MODE != 0)
// 状态函数句柄的定义
struct eos_sm;
typedef eos_ret_t (* eos_state_handler)(struct eos_sm *const me,
                                        eos_event_t const * const e);
#endif

#if (EOS_USE_SM_MODE != 0)
// 状态机类
typedef struct eos_sm {
    eos_task_t super;
    volatile eos_state_handler state;
} eos_sm_t;
#endif

#if (EOS_USE_SM_MODE != 0)
// 状态机初始化函数
void eos_sm_init(   eos_sm_t * const me,
                    const char *name,
                    uint8_t priority,
                    void *stack, uint32_t size);
void eos_sm_start(eos_sm_t * const me, eos_state_handler state_init);

eos_ret_t eos_tran(eos_sm_t * const me, eos_state_handler state);
eos_ret_t eos_super(eos_sm_t * const me, eos_state_handler state);
eos_ret_t eos_state_top(eos_sm_t * const me, eos_event_t const * const e);

#define EOS_TRAN(target)            eos_tran((eos_sm_t * )me, (eos_state_handler)target)
#define EOS_SUPER(super)            eos_super((eos_sm_t * )me, (eos_state_handler)super)
#define EOS_STATE_CAST(state)       ((eos_state_handler)(state))
#endif

/* -----------------------------------------------------------------------------
Trace
----------------------------------------------------------------------------- */
#if (EOS_USE_STACK_USAGE != 0)
// 任务的堆栈使用率
uint8_t eos_task_stack_usage(uint8_t priority);
#endif

#if (EOS_USE_CPU_USAGE != 0)
// 任务的CPU使用率
uint8_t eos_task_cpu_usage(uint8_t priority);
// 监控函数，放进一个单独的定时器中断函数，中断频率为SysTick的10-20倍。
void eos_cpu_usage_monitor(void);
#endif

/* -----------------------------------------------------------------------------
Port
----------------------------------------------------------------------------- */
void eos_port_task_switch(void);
void eos_port_assert(uint32_t error_id);

/* -----------------------------------------------------------------------------
Hook
----------------------------------------------------------------------------- */
// 空闲回调函数
void eos_hook_idle(void);

// 结束EventOS的运行的时候，所调用的回调函数。
void eos_hook_stop(void);

// 启动EventOS的时候，所调用的回调函数
void eos_hook_start(void);

#ifdef __cplusplus
}
#endif

#endif
