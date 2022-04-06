
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
#ifndef EOS_MAX_ACTORS
#define EOS_MAX_ACTORS                          8       // 默认最多8个Actor
#endif

#ifndef EOS_MAX_EVENTS
#define EOS_MAX_EVENTS                          256     // 默认最多256个事件
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
#include "eventos_def.h"

/* -----------------------------------------------------------------------------
EventOS
----------------------------------------------------------------------------- */
// EventOS initialization.
void eos_init(void);
// Run EventOS.
void eos_run(void);
// System time.
uint32_t eos_time(void);
// System tick function.
void eos_tick(void);
// 关闭中断
void eos_critical_enter(void);
// 开中断
void eos_critical_exit(void);

/* -----------------------------------------------------------------------------
Task
----------------------------------------------------------------------------- */
/*
 * Defines the prototype to which the application task hook function must
 * conform.
 */
typedef void (* eos_func_t)(void);

/*
 * Definition of the task class.
 */
typedef struct eos_task {
    uint32_t *sp;
    void *stack;
    uint32_t size;
    uint32_t timeout;
    uint32_t stack_size;              /* stack size */
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
void eos_delay_unsub_event(uint32_t time_ms);
// 挂起某任务
void eos_task_suspend(const char *task);
// 删除某任务
void eos_task_delete(const char *task);
// 恢复某任务
void eos_task_resume(const char *task);
// 任务等待某事件
void eos_task_wait_event(const char *event);
// 任务取消等待
void eos_task_wait_cancel(void);

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
    uint32_t id                     : 10;
    uint32_t domain                 : 8;
    uint32_t oneshoot               : 1;
    uint32_t running                : 1;
} eos_timer_t;

// 启动软定时器，允许在中断中调用。
int32_t eos_timer_start(eos_timer_t * const me,
                        uint32_t time_ms,
                        bool oneshoot,
                        eos_func_t callback);
// 删除软定时器，允许在中断中调用。
void eos_timer_delete(uint16_t timer_id);
// 暂停软定时器，允许在中断中调用。
void eos_timer_pause(uint16_t timer_id);
// 继续软定时器，允许在中断中调用。
void eos_timer_continue(uint16_t timer_id);
// 重启软定时器的定时，允许在中断中调用。
void eos_timer_reset(uint16_t timer_id);

/* -----------------------------------------------------------------------------
Event
----------------------------------------------------------------------------- */
/*
 * Definition of the event class.
 */
typedef struct eos_event {
    const char *topic;                      // 事件主题
    void *data;                             // 事件数据
    uint32_t size;                          // 数据长度
} eos_event_t;

// 检查事件的主题 -----------------------------------------
bool eos_event_topic(eos_event_t const * const e, const char *topic);

// 事件的属性设置 -----------------------------------------
void eos_event_set_global(const char *topic);
// 设置不可阻塞事件。在延时时，此类事件进入，延时结束，对此类事件进行立即响应。
void eos_event_set_unblocked(const char *topic);
void eos_event_set_stream(const char *topic);

// 事件的直接发送 -----------------------------------------
void eos_event_send_topic(const char *task, const char *topic);
void eos_event_send_value(const char *task, const char *topic, void const *data);
void eos_event_send_stream(const char *task, const char *topic, void const *data, uint32_t size);

// 事件的广播 --------------------------------------------
void eos_event_broadcast_topic(const char *topic);
void eos_event_broadcast_value(const char *topic, void const *data);

// 事件的发布 --------------------------------------------
// 注：只有下面两个函数能在中断服务函数中使用，其他都没有必要。如果使用，可能会导致崩溃问题。
// 发布事件（仅主题）
void eos_event_pub_topic(const char *topic);
void eos_event_pub_delay(const char *topic, uint32_t time_delay_ms);
// 发布事件（携带数据）
void eos_event_pub_value(const char *topic, void *data);
void eos_event_pub_period(const char *topic, uint32_t time_period_ms);
void eos_event_time_cancel(const char *topic);

// 事件的订阅 --------------------------------------------
// 事件订阅
void eos_event_sub(eos_task_t *const me, const char *topic);
// 事件取消订阅
void eos_event_unsub(eos_task_t *const me, const char *topic);
// 事件订阅宏定义
#define EOS_EVENT_SUB(_evt)               eos_event_sub(&(me->super.super), _evt)
// 事件取消订阅宏定义
#define EOS_EVENT_UNSUB(_evt)             eos_event_unsub(&(me->super.super), _evt)

/* -----------------------------------------------------------------------------
Reactor
----------------------------------------------------------------------------- */
// 事件处理句柄的定义
struct eos_reactor;
typedef void (* eos_event_handler)(struct eos_reactor *const me, eos_event_t const * const e);

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
// 状态返回值的定义
#if (EOS_USE_SM_MODE != 0)
typedef enum eos_ret {
    EOS_Ret_Null = 0,                       // 无效值
    EOS_Ret_Handled,                        // 已处理，不产生跳转
    EOS_Ret_Super,                          // 到超状态
    EOS_Ret_Tran,                           // 跳转
} eos_ret_t;
#endif

#if (EOS_USE_SM_MODE != 0)
// 状态函数句柄的定义
struct eos_sm;
typedef eos_ret_t (* eos_state_handler)(struct eos_sm *const me, eos_event_t const * const e);
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
