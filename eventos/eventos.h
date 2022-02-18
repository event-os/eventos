
/*
 * EventOS
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
// 系统事件、时间事件与局部事件区的定义
enum eos_event_topic {
    Event_Null = 0,
    Event_Enter,
    Event_Exit,
    Event_Init,
    Event_User,
};

typedef eos_u16_t                       eos_topic_t;

typedef union eos_time {
    eos_u32_t day           : 5;
    eos_u32_t hour          : 5;
    eos_u32_t minute        : 6;
    eos_u32_t second        : 6;
    eos_u32_t ms            : 10;
} eos_time_t;

// 状态返回值的定义
typedef enum eos_ret {
    EOS_Ret_Null = 0,                       // 无效值
    EOS_Ret_Super,                          // 到超状态
    EOS_Ret_Handled,                        // 已处理，不产生跳转
    EOS_Ret_Tran,                           // 跳转
} eos_ret_t;

// 带16字节参数的事件类
typedef struct eos_event {
    eos_s32_t topic;
    eos_u32_t flag_sub;
    eos_u8_t para[EOS_EVENT_PARAS_NUM];
} eos_event_t;

// 数据结构 - 行为树相关 --------------------------------------------------------
struct eos_sm;
// 状态函数句柄的定义
typedef eos_ret_t (* eos_state_handler)(struct eos_sm *const me, eos_event_t const * const e);

// 行为对象类
typedef struct eos_sm {
    eos_u32_t magic;
    volatile eos_state_handler state;
    // volatile eos_state_handler state_tgt;
    // evt queue
    eos_topic_t* e_queue;
    volatile eos_topic_t head;
    volatile eos_topic_t tail;
    volatile eos_topic_t depth;
    eos_u8_t priv;
    eos_bool_t enabled;
    volatile eos_bool_t equeue_empty;
} eos_sm_t;

// api -------------------------------------------------------------------------
// 对框架进行初始化，在各状态机初始化之前调用。
void eventos_init(eos_u32_t *flag_sub);
// 启动框架，放在main函数的末尾。
eos_s32_t eventos_run(void);
// 停止框架的运行（不常用）
// 停止框架后，框架会在执行完当前状态机的当前事件后，清空各状态机事件队列，清空事件池，
// 不再执行任何功能，直至框架被再次启动。
void eventos_stop(void);

// 关于状态机 -----------------------------------------------
// 状态机初始化函数
void eos_sm_init(   eos_sm_t * const me,
                    eos_u32_t priority,
                    void *memory_queue, eos_u32_t queue_size,
                    void *memory_stack, eos_u32_t stask_size);
void eos_sm_start(eos_sm_t * const me, eos_state_handler state_init);

// 关于事件 -------------------------------------------------
void eos_event_sub(eos_sm_t * const me, eos_topic_t topic);
void eos_event_unsub(eos_sm_t * const me, eos_topic_t topic);
// 注：只有下面两个函数能在中断服务函数中使用，其他都没有必要。如果使用，可能会导致崩溃问题。
void eos_event_pub_topic(eos_topic_t topic);
void eos_event_pub(eos_topic_t topic, void *data, eos_u32_t size);
void eos_event_pub_delay(eos_topic_t topic, eos_u32_t time_ms);
void eos_event_pub_period(eos_topic_t topic, eos_u32_t time_ms);

#define EOS_EVENT_SUB(_evt)               eos_event_sub(&(me->super), _evt)
#define EOS_EVENT_UNSUB(_evt)             eos_event_unsub(&(me->super), _evt)

// 关于状态 -------------------------------------------------
eos_ret_t eos_tran(eos_sm_t * const me, eos_state_handler state);
eos_ret_t eos_super(eos_sm_t * const me, eos_state_handler state);
eos_ret_t eos_state_top(eos_sm_t * const me, eos_event_t const * const e);

#define EOS_TRAN(target)            eos_tran((eos_sm_t * )me, (eos_state_handler)target)
#define EOS_SUPER(super)            eos_super((eos_sm_t * )me, (eos_state_handler)super)
#define EOS_STATE_CAST(state)       (eos_state_handler)(state)

/* port --------------------------------------------------------------------- */
eos_u32_t eos_port_get_time_ms(void);
void eos_port_critical_enter(void);
void eos_port_critical_exit(void);
void eos_port_assert(eos_u32_t error_id);

/* hook --------------------------------------------------------------------- */
void eos_hook_idle(void);
void eos_hook_stop(void);
void eos_hook_start(void);

/* assert ------------------------------------------------------------------- */
#define EOS_ASSERT(test_) do { if (!(test_)) {                                 \
    eos_hook_stop(); eos_port_assert(__LINE__); } } while (0)

#define EOS_ASSERT_ID(id_, test_) do { if (!(test_)) {                         \
    eos_hook_stop(); eos_port_assert(id_); } } while (0);

#ifdef __cplusplus
}
#endif

#endif
