
/*
 * EventOS Nano
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
 * https://github.com/event-os/eventos-nano
 * https://gitee.com/event-os/eventos-nano
 * 
 * Change Logs:
 * Date           Author        Notes
 * 2021-11-23     DogMing       V0.0.2
 */

#ifndef EVENTOS_H_
#define EVENTOS_H_

/* include ------------------------------------------------------------------ */
#include "eventos_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* default configuration ---------------------------------------------------- */
#ifndef EOS_MCU_TYPE
#define EOS_MCU_TYPE                            32      // 默认32位单片机
#endif

#ifndef EOS_MAX_ACTORS
#define EOS_MAX_ACTORS                          8      // 默认最多8个Actor
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

#include "eventos_def.h"

/* data struct -------------------------------------------------------------- */
enum eos_event_topic {
#if (EOS_USE_SM_MODE != 0)
    Event_Null = 0,
    Event_Enter,
    Event_Exit,
#if (EOS_USE_HSM_MODE != 0)
    Event_Init,
#endif
    Event_User,
#else
    Event_Null = 0,
    Event_User,
#endif
};

#if (EOS_MCU_TYPE == 8)
typedef eos_u8_t                        eos_topic_t;
#else
typedef eos_u16_t                       eos_topic_t;
#endif

// 状态返回值的定义
#if (EOS_USE_SM_MODE != 0)
typedef enum eos_ret {
    EOS_Ret_Null = 0,                       // 无效值
    EOS_Ret_Handled,                        // 已处理，不产生跳转
    EOS_Ret_Super,                          // 到超状态
    EOS_Ret_Tran,                           // 跳转
} eos_ret_t;
#endif

// 事件类
typedef struct eos_event {
    eos_topic_t topic;
    void *data;
    eos_u16_t size;
} eos_event_t;

// 数据结构 - 行为树相关 --------------------------------------------------------
typedef void (* eos_func_t)(void);

// 事件处理句柄的定义
struct eos_reactor;
typedef void (* eos_event_handler)(struct eos_reactor *const me, eos_event_t const * const e);

#if (EOS_USE_SM_MODE != 0)
// 状态函数句柄的定义
struct eos_sm;
typedef eos_ret_t (* eos_state_handler)(struct eos_sm *const me, eos_event_t const * const e);
#endif

typedef eos_event_t *                       eos_event_quote_t;

// Actor类
typedef struct eos_actor {
    eos_u32_t *sp;
    void *stack;
    eos_u32_t size;
    eos_u32_t timeout;
#if (EOS_MCU_TYPE == 32 || EOS_MCU_TYPE == 16)
    eos_u32_t stack_size            : 16;              /* stack size */
    eos_u32_t priority              : 5;
    eos_u32_t mode                  : 1;
    eos_u32_t enabled               : 1;
    eos_u32_t reserve               : 1;
#else
    eos_u8_t priority               : 5;
    eos_u8_t mode                   : 1;
    eos_u8_t enabled                : 1;
    eos_u8_t reserve                : 1;
#endif
} eos_actor_t;

// React类
typedef struct eos_reactor {
    eos_actor_t super;
    eos_event_handler event_handler;
} eos_reactor_t;

#if (EOS_USE_SM_MODE != 0)
// 状态机类
typedef struct eos_sm {
    eos_actor_t super;
    volatile eos_state_handler state;
} eos_sm_t;
#endif

// api -------------------------------------------------------------------------
// 对框架进行初始化，在各状态机初始化之前调用。
void eos_init(void);
#if (EOS_USE_PUB_SUB != 0)
void eos_sub_init(eos_mcu_t *flag_sub, eos_topic_t topic_max);
#endif
// 启动框架，放在main函数的末尾。
void eos_run(void);
// 停止框架的运行（不常用）
// 停止框架后，框架会在执行完当前状态机的当前事件后，清空各状态机事件队列，清空事件池，
// 不再执行任何功能，直至框架被再次启动。
void eos_stop(void);
#if (EOS_USE_TIME_EVENT != 0)
eos_u32_t eos_time(void);
void eos_tick(void);
#endif
void eos_delay_ms(eos_u32_t time_ms);

// 关于Reactor -----------------------------------------------------------------
void eos_reactor_init(  eos_reactor_t * const me,
                        eos_u8_t priority,
                        void *stack, eos_u32_t size);
void eos_reactor_start(eos_reactor_t * const me, eos_event_handler event_handler);
#define EOS_HANDLER_CAST(handler)       ((eos_event_handler)(handler))

// 关于状态机 -----------------------------------------------
#if (EOS_USE_SM_MODE != 0)
// 状态机初始化函数
void eos_sm_init(   eos_sm_t * const me,
                    eos_u8_t priority,
                    void *stack, eos_u32_t size);
void eos_sm_start(eos_sm_t * const me, eos_state_handler state_init);

eos_ret_t eos_tran(eos_sm_t * const me, eos_state_handler state);
eos_ret_t eos_super(eos_sm_t * const me, eos_state_handler state);
eos_ret_t eos_state_top(eos_sm_t * const me, eos_event_t const * const e);

#define EOS_TRAN(target)            eos_tran((eos_sm_t * )me, (eos_state_handler)target)
#define EOS_SUPER(super)            eos_super((eos_sm_t * )me, (eos_state_handler)super)
#define EOS_STATE_CAST(state)       ((eos_state_handler)(state))
#endif

// 关于事件 -------------------------------------------------
#if (EOS_USE_PUB_SUB != 0)
void eos_event_sub(eos_actor_t * const me, eos_topic_t topic);
void eos_event_unsub(eos_actor_t * const me, eos_topic_t topic);
#define EOS_EVENT_SUB(_evt)               eos_event_sub(&(me->super.super), _evt)
#define EOS_EVENT_UNSUB(_evt)             eos_event_unsub(&(me->super.super), _evt)
#endif

// 注：只有下面两个函数能在中断服务函数中使用，其他都没有必要。如果使用，可能会导致崩溃问题。
void eos_event_pub_topic(eos_topic_t topic);
#if (EOS_USE_EVENT_DATA != 0)
void eos_event_pub(eos_topic_t topic, void *data, eos_u32_t size);
#endif

#if (EOS_USE_TIME_EVENT != 0)
void eos_event_pub_delay(eos_topic_t topic, eos_u32_t delay_time_ms);
void eos_event_pub_period(eos_topic_t topic, eos_u32_t peroid_ms);
void eos_event_time_cancel(eos_topic_t topic);
#endif

/* port --------------------------------------------------------------------- */
void eos_port_critical_enter(void);
void eos_port_critical_exit(void);
void eos_port_task_switch(void);
void eos_port_assert(eos_u32_t error_id);

/* hook --------------------------------------------------------------------- */
void eos_hook_idle(void);
void eos_hook_stop(void);
void eos_hook_start(void);

#ifdef __cplusplus
}
#endif

#endif
