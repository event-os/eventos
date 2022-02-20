
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
 * https://github.com/eventos-nano
 * https://gitee.com/eventos-nano
 * 
 * Change Logs:
 * Date           Author        Notes
 * 2021-11-23     Lao Wang      V0.0.2
 */

#ifndef EVENTOS_H_
#define EVENTOS_H_

/* include ------------------------------------------------------------------ */
#include "eventos_def.h"
#include "eventos_config.h"

#ifdef __cplusplus
extern "C" {
#endif

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
    Event_User = 0,
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
    eos_u32_t topic;
    void *data;
} eos_event_t;

// 数据结构 - 行为树相关 --------------------------------------------------------
// 事件处理句柄的定义
struct eos_reactor;
typedef void (* eos_event_handler)(struct eos_reactor *const me, eos_event_t const * const e);

#if (EOS_USE_SM_MODE != 0)
// 状态函数句柄的定义
struct eos_sm;
typedef eos_ret_t (* eos_state_handler)(struct eos_sm *const me, eos_event_t const * const e);
#endif

// Actor类
typedef struct eos_actor {
    eos_u32_t magic;
    // evt queue
    void* e_queue;
    volatile eos_topic_t head;
    volatile eos_topic_t tail;
    volatile eos_topic_t depth;
    eos_u8_t priority;
    eos_u8_t mode;
    eos_bool_t enabled;
    volatile eos_bool_t equeue_empty;
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
void eventos_init(void);
#if (EOS_USE_PUB_SUB != 0)
void eos_sub_init(eos_mcu_t *flag_sub, eos_topic_t topic_max);
#endif
// 启动框架，放在main函数的末尾。
void eventos_run(void);
// 停止框架的运行（不常用）
// 停止框架后，框架会在执行完当前状态机的当前事件后，清空各状态机事件队列，清空事件池，
// 不再执行任何功能，直至框架被再次启动。
void eventos_stop(void);

// 关于Reactor -----------------------------------------------------------------
void eos_reactor_init(  eos_reactor_t * const me,
                        eos_u32_t priority,
                        void *memory_queue, eos_u32_t queue_size);
void eos_reactor_start(eos_reactor_t * const me, eos_event_handler event_handler);


// 关于状态机 -----------------------------------------------
#if (EOS_USE_SM_MODE != 0)
// 状态机初始化函数
void eos_sm_init(   eos_sm_t * const me,
                    eos_u32_t priority,
                    void *memory_queue, eos_u32_t queue_size);
void eos_sm_start(eos_sm_t * const me, eos_state_handler state_init);

eos_ret_t eos_tran(eos_sm_t * const me, eos_state_handler state);
eos_ret_t eos_super(eos_sm_t * const me, eos_state_handler state);
eos_ret_t eos_state_top(eos_sm_t * const me, eos_event_t const * const e);

#define EOS_TRAN(target)            eos_tran((eos_sm_t * )me, (eos_state_handler)target)
#define EOS_SUPER(super)            eos_super((eos_sm_t * )me, (eos_state_handler)super)
#define EOS_STATE_CAST(state)       (eos_state_handler)(state)
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
void eos_event_pub_delay(eos_topic_t topic, eos_u32_t time_ms);
void eos_event_pub_period(eos_topic_t topic, eos_u32_t time_ms);
#endif

/* port --------------------------------------------------------------------- */
#if (EOS_USE_TIME_EVENT != 0)
eos_u32_t eos_port_get_time_ms(void);
#endif
void eos_port_critical_enter(void);
void eos_port_critical_exit(void);
void eos_port_assert(eos_u32_t error_id);

/* hook --------------------------------------------------------------------- */
void eos_hook_idle(void);
void eos_hook_stop(void);
void eos_hook_start(void);

#ifdef __cplusplus
}
#endif

#endif
