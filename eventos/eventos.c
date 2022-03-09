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

// include ---------------------------------------------------------------------
#include "eventos.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* assert ------------------------------------------------------------------- */
#if (EOS_USE_ASSERT != 0)
#define EOS_ASSERT(test_) do { if (!(test_)) {                                 \
        eos_hook_stop();                                                       \
        eos_port_critical_enter();                                             \
        eos_port_assert(__LINE__);                                             \
    } } while (0)
#else
#define EOS_ASSERT(test_)               ((void)0)
#endif

// eos define ------------------------------------------------------------------
enum eos_actor_mode {
    EOS_Mode_Reactor = 0,
    EOS_Mode_StateMachine = !EOS_Mode_Reactor
};

// **eos** ---------------------------------------------------------------------
enum {
    EosRun_OK                               = 0,
    EosRun_NotEnabled,
    EosRun_NoEvent,
    EosRun_NoActor,
    EosRun_NoActorSub,

    // Timer
    EosTimer_Empty,
    EosTimer_NotTimeout,
    EosTimer_ChangeToEmpty,

    EosRunErr_NotInitEnd                    = -1,
    EosRunErr_ActorNotSub                   = -2,
    EosRunErr_MallocFail                    = -3,
    EosRunErr_SubTableNull                  = -4,
    EosRunErr_InvalidEventData              = -5,
    EosRunErr_HeapMemoryNotEnough           = -6,
    EosRunErr_TimerRepeated                 = -7,
};

#if (EOS_USE_TIME_EVENT != 0)
#define EOS_MS_NUM_30DAY                    (2592000000)

enum {
    EosTimerUnit_Ms                         = 0,    // 60S, ms
    EosTimerUnit_100Ms,                             // 100Min, 50ms
    EosTimerUnit_Sec,                               // 16h, 500ms
    EosTimerUnit_Minute,                            // 15day, 30S

    EosTimerUnit_Max
};

static const eos_u32_t timer_threshold[EosTimerUnit_Max] = {
    60000,                                          // 60 S
    6000000,                                        // 100 Minutes
    57600000,                                       // 16 hours
    1296000000,                                     // 15 days
};

static const eos_u32_t timer_unit[EosTimerUnit_Max] = {
    1, 100, 1000, 60000
};

typedef struct eos_event_timer {
    eos_u32_t topic                         : 13;
    eos_u32_t oneshoot                      : 1;
    eos_u32_t unit                          : 2;
    eos_u32_t period                        : 16;
    eos_u32_t timeout_ms;
} eos_event_timer_t;
#endif

typedef struct eos_block {
    // word[0]
    eos_u32_t next                          : 15;
    eos_u32_t q_next                        : 15;
    eos_u32_t offset                        : 2;
    // word[1]
    eos_u32_t last                          : 15;
    eos_u32_t q_last                        : 15;
    eos_u32_t free                          : 1;
    // word[2]
    eos_u16_t size                          : 15;
} eos_block_t;

typedef struct eos_event_inner {
    eos_sub_t sub;
    eos_topic_t topic;
} eos_event_inner_t;

typedef struct eos_heap {
    eos_u8_t data[EOS_SIZE_HEAP];
    // word[0]
    eos_u32_t size                          : 15;       /* total size */
    eos_u32_t queue                         : 15;
    eos_u32_t error_id                      : 2;
    // word[2]
    eos_u32_t current                       : 15;
    eos_u32_t empty                         : 1;
    // word[2]
    eos_sub_t sub_general;
    eos_sub_t count;
} eos_heap_t;

typedef struct eos_tag {
#if (EOS_USE_PUB_SUB != 0)
    eos_mcu_t *sub_table;                                     // event sub table
#endif

    eos_mcu_t actor_exist;
    eos_mcu_t actor_enabled;
    eos_actor_t * actor[EOS_MAX_ACTORS];

#if (EOS_USE_EVENT_DATA != 0)
    eos_heap_t heap;
#endif

#if (EOS_USE_TIME_EVENT != 0)
    eos_event_timer_t etimer[EOS_MAX_TIME_EVENT];
    eos_u32_t time;
    eos_u32_t timeout_min;
    eos_u8_t timer_count;
#endif

    eos_u8_t enabled                        : 1;
    eos_u8_t running                        : 1;
    eos_u8_t init_end                       : 1;
} eos_t;

/* eventos API for test ----------------------------- */
eos_s8_t eos_once(void);
eos_s8_t eos_event_pub_ret(eos_topic_t topic, void *data, eos_u32_t size);
void * eos_get_framework(void);
void eos_event_pub_time(eos_topic_t topic, eos_u32_t time_ms, eos_bool_t oneshoot);
void eos_set_time(eos_u32_t time_ms);
// **eos end** -----------------------------------------------------------------

static eos_t eos;

// data ------------------------------------------------------------------------
#if (EOS_USE_SM_MODE != 0)
static const eos_event_t eos_event_table[Event_User] = {
    {Event_Null, 0},
    {Event_Enter, 0},
    {Event_Exit, 0},
#if (EOS_USE_HSM_MODE != 0)
    {Event_Init, 0},
#endif
};
#endif

// macro -----------------------------------------------------------------------
#if (EOS_USE_SM_MODE != 0)
#define HSM_TRIG_(state_, topic_)                                              \
    ((*(state_))(me, &eos_event_table[topic_]))
#endif

// static function -------------------------------------------------------------
#if (EOS_USE_SM_MODE != 0)
static void eos_sm_dispath(eos_sm_t * const me, eos_event_t const * const e);
#if (EOS_USE_HSM_MODE != 0)
static eos_s32_t eos_sm_tran(eos_sm_t * const me, eos_state_handler path[EOS_MAX_HSM_NEST_DEPTH]);
#endif
#endif
#if (EOS_USE_EVENT_DATA != 0)
void eos_heap_init(eos_heap_t * const me);
void * eos_heap_malloc(eos_heap_t * const me, eos_u32_t size);
void eos_heap_free(eos_heap_t * const me, void * data);
void *eos_heap_get_block(eos_heap_t * const me, eos_u8_t priority);
void eos_heap_gc(eos_heap_t * const me, void *data);
#endif

// eventos ---------------------------------------------------------------------
static void eos_clear(void)
{
#if (EOS_USE_TIME_EVENT != 0)
    eos.timer_count = 0;
#endif
}

void eos_init(void)
{
    eos_clear();

    eos.enabled = EOS_True;
    eos.running = EOS_False;
    eos.actor_exist = 0;
    eos.actor_enabled = 0;
#if (EOS_USE_PUB_SUB != 0)
    eos.sub_table = EOS_NULL;
#endif

#if (EOS_USE_EVENT_DATA != 0)
    eos_heap_init(&eos.heap);
#endif

    eos.init_end = 1;
#if (EOS_USE_TIME_EVENT != 0)
    eos.time = 0;
#endif
}

#if (EOS_USE_PUB_SUB != 0)
void eos_sub_init(eos_mcu_t *flag_sub, eos_topic_t topic_max)
{
    eos.sub_table = flag_sub;
    for (int i = 0; i < topic_max; i ++) {
        eos.sub_table[i] = 0;
    }
}
#endif

#if (EOS_USE_TIME_EVENT != 0)
eos_s32_t eos_evttimer(void)
{
    // 获取当前时间，检查延时事件队列
    eos_u32_t system_time = eos.time;
    
    if (eos.etimer[0].topic == Event_Null)
        return EosTimer_Empty;

    // 时间未到达
    if (system_time < eos.timeout_min)
        return EosTimer_NotTimeout;
    // 若时间到达，将此事件推入事件队列，同时在etimer里删除。
    for (eos_u32_t i = 0; i < eos.timer_count; i ++) {
        if (eos.etimer[i].timeout_ms > system_time)
            continue;
        eos_event_pub_topic(eos.etimer[i].topic);
        // 清零标志位
        if (eos.etimer[i].oneshoot == EOS_True) {
            if (i == (eos.timer_count - 1)) {
                eos.timer_count -= 1;
                break;
            }
            eos.etimer[i] = eos.etimer[eos.timer_count - 1];
            eos.timer_count -= 1;
            i --;
        }
        else {
            if (eos.etimer[i].unit == 1) {
                eos.etimer[i].timeout_ms += eos.etimer[i].period;
            }
            else {
                eos_u32_t period = eos.etimer[i].period * timer_unit[eos.etimer[i].unit];
                eos.etimer[i].timeout_ms += period;
            }
        }
    }
    if (eos.timer_count == 0) {
        eos.timeout_min = EOS_U32_MAX;
        return EosTimer_ChangeToEmpty;
    }

    // 寻找到最小的时间定时器
    eos_u32_t min_time_out_ms = EOS_U32_MAX;
    for (eos_u32_t i = 0; i < eos.timer_count; i ++) {
        if (min_time_out_ms <= eos.etimer[i].timeout_ms)
            continue;
        min_time_out_ms = eos.etimer[i].timeout_ms;
    }
    eos.timeout_min = min_time_out_ms;

    return EosRun_OK;
}
#endif

eos_s8_t eos_once(void)
{
    if (eos.init_end == 0) {
        return (eos_s8_t)EosRunErr_NotInitEnd;
    }

#if (EOS_USE_PUB_SUB != 0)
    if (eos.sub_table == EOS_NULL) {
        return (eos_s8_t)EosRunErr_SubTableNull;
    }
#endif

    if (eos.enabled == EOS_False) {
        eos_clear();
        return (eos_s8_t)EosRun_NotEnabled;
    }

    // 检查是否有状态机的注册
    if (eos.actor_exist == 0 || eos.actor_enabled == 0) {
        return (eos_s8_t)EosRun_NoActor;
    }

#if (EOS_USE_TIME_EVENT != 0)
    eos_evttimer();
#endif

    if (eos.heap.empty == EOS_True) {
        return (eos_s8_t)EosRun_NoEvent;
    }

    // 寻找到优先级最高，且有事件需要处理的Actor
    eos_actor_t *actor = (eos_actor_t *)0;
    eos_u8_t priority = EOS_MAX_ACTORS;
    for (eos_s8_t i = (eos_s8_t)(EOS_MAX_ACTORS - 1); i >= 0; i --) {
        if ((eos.actor_exist & (1 << i)) == 0)
            continue;
        if ((eos.heap.sub_general & (1 << i)) == 0)
            continue;
        actor = eos.actor[i];
        priority = i;
        break;
    }
    // 如果没有找到，返回
    if (priority == EOS_MAX_ACTORS) {
        return (eos_s8_t)EosRun_NoActorSub;
    }

    // 寻找当前Actor的最老的事件
    eos_port_critical_enter();
    eos_event_inner_t * e = eos_heap_get_block(&eos.heap, priority);
    EOS_ASSERT(e != EOS_NULL);

    eos_port_critical_exit();
    eos_event_t event;
    event.topic = e->topic;
    event.data = (void *)((eos_pointer_t)e + sizeof(eos_event_inner_t));
    // 对事件进行执行
#if (EOS_USE_PUB_SUB != 0)
    if ((eos.sub_table[e->topic] & (1 << actor->priority)) != 0)
#endif
    {
#if (EOS_USE_SM_MODE != 0)
        if (actor->mode == EOS_Mode_StateMachine) {
            // 执行状态的转换
            eos_sm_t *sm = (eos_sm_t *)actor;
            eos_sm_dispath(sm, &event);
        }
        else 
#endif
        {
            eos_reactor_t *reactor = (eos_reactor_t *)actor;
            reactor->event_handler(reactor, &event);
        }
    }
#if (EOS_USE_PUB_SUB != 0)
    else {
        return (eos_s8_t)EosRunErr_ActorNotSub;
    }
#endif
#if (EOS_USE_EVENT_DATA != 0)
    // 销毁过期事件与其携带的参数
    eos_port_critical_enter();
    eos_heap_gc(&eos.heap, e);
    eos_port_critical_exit();
#endif

    return (eos_s8_t)EosRun_OK;
}

void eos_run(void)
{
    eos_hook_start();

    EOS_ASSERT(eos.enabled == EOS_True);
#if (EOS_USE_PUB_SUB != 0)
    EOS_ASSERT(eos.sub_table != 0);
#endif
#if (EOS_USE_EVENT_DATA != 0 && EOS_USE_HEAP != 0)
    EOS_ASSERT(eos.heap.size != 0);
#endif

    eos.running = EOS_True;

    while (eos.enabled) {
        eos_s8_t ret = eos_once();
        EOS_ASSERT(ret >= 0);

        if (ret == EosRun_NotEnabled) {
            break;
        }

        if (ret == EosRun_NoActor || ret == EosRun_NoEvent) {
            eos_hook_idle();
        }
    }

    while (1) {
        eos_hook_idle();
    }
}

void eos_stop(void)
{
    eos.enabled = EOS_False;
    eos_hook_stop();
}

#if (EOS_USE_TIME_EVENT != 0)
eos_u32_t eos_time(void)
{
    return eos.time;
}

void eos_tick(void)
{
    eos_u32_t system_time = eos.time, system_time_bkp = eos.time;
    eos_u32_t offset = EOS_MS_NUM_30DAY - 1 + EOS_TICK_MS;
    system_time = ((system_time + EOS_TICK_MS) % EOS_MS_NUM_30DAY);
    if (system_time_bkp >= (EOS_MS_NUM_30DAY - EOS_TICK_MS) && system_time < EOS_TICK_MS) {
        eos_port_critical_enter();
        EOS_ASSERT(eos.timeout_min >= offset);
        eos.timeout_min -= offset;
        for (eos_u32_t i = 0; i < eos.timer_count; i ++) {
            EOS_ASSERT(eos.etimer[i].timeout_ms >= offset);
            eos.etimer[i].timeout_ms -= offset;
        }
        eos_port_critical_exit();
    }
    eos.time = system_time;
}
#endif

// 关于Reactor -----------------------------------------------------------------
static void eos_actor_init( eos_actor_t * const me,
                            eos_u8_t priority,
                            void const * const parameter)
{
    (void)parameter;

    // 框架需要先启动起来
    EOS_ASSERT(eos.enabled == EOS_True);
    EOS_ASSERT(eos.running == EOS_False);
#if (EOS_USE_PUB_SUB != 0)
    EOS_ASSERT(eos.sub_table != EOS_NULL);
#endif
    // 参数检查
    EOS_ASSERT(me != (eos_actor_t *)0);
    EOS_ASSERT(priority < EOS_MAX_ACTORS);

    // 防止二次启动
    if (me->enabled == EOS_True)
        return;

    // 检查优先级的重复注册
    EOS_ASSERT((eos.actor_exist & (1 << priority)) == 0);

    // 注册到框架里
    eos.actor_exist |= (1 << priority);
    eos.actor[priority] = me;
    // 状态机   
    me->priority = priority;
}

void eos_reactor_init(  eos_reactor_t * const me,
                        eos_u8_t priority,
                        void const * const parameter)
{
    eos_actor_init(&me->super, priority, parameter);
    me->super.mode = EOS_Mode_Reactor;
}

void eos_reactor_start(eos_reactor_t * const me, eos_event_handler event_handler)
{
    me->event_handler = event_handler;
    me->super.enabled = EOS_True;
    eos.actor_enabled |= (1 << me->super.priority);
}

// state machine ---------------------------------------------------------------
#if (EOS_USE_SM_MODE != 0)
void eos_sm_init(   eos_sm_t * const me,
                    eos_u8_t priority,
                    void const * const parameter)
{
    eos_actor_init(&me->super, priority, parameter);
    me->super.mode = EOS_Mode_StateMachine;
    me->state = eos_state_top;
}

void eos_sm_start(eos_sm_t * const me, eos_state_handler state_init)
{
#if (EOS_USE_HSM_MODE != 0)
    eos_state_handler path[EOS_MAX_HSM_NEST_DEPTH];
#endif
    eos_state_handler t;

    me->state = state_init;
    me->super.enabled = EOS_True;
    eos.actor_enabled |= (1 << me->super.priority);

    // 进入初始状态，执行TRAN动作。这也意味着，进入初始状态，必须无条件执行Tran动作。
    t = me->state;
    eos_ret_t ret = t(me, &eos_event_table[Event_Null]);
    EOS_ASSERT(ret == EOS_Ret_Tran);
#if (EOS_USE_HSM_MODE == 0)
    ret = me->state(me, &eos_event_table[Event_Enter]);
    EOS_ASSERT(ret != EOS_Ret_Tran);
#else
    t = eos_state_top;
    // 由初始状态转移，引发的各层状态的进入
    // 每一个循环，都代表着一个Event_Init的执行
    eos_s32_t ip = 0;
    ret = EOS_Ret_Null;
    do {
        // 由当前层，探测需要进入的各层父状态
        path[0] = me->state;
        // 一层一层的探测，一直探测到原状态
        HSM_TRIG_(me->state, Event_Null);
        while (me->state != t) {
            ++ ip;
            EOS_ASSERT(ip < EOS_MAX_HSM_NEST_DEPTH);
            path[ip] = me->state;
            HSM_TRIG_(me->state, Event_Null);
        }
        me->state = path[0];

        // 各层状态的进入
        do {
            HSM_TRIG_(path[ip --], Event_Enter);
        } while (ip >= 0);

        t = path[0];

        ret = HSM_TRIG_(t, Event_Init);
    } while (ret == EOS_Ret_Tran);

    me->state = t;
#endif
}
#endif

// event -----------------------------------------------------------------------
eos_s8_t eos_event_pub_ret(eos_topic_t topic, void *data, eos_u32_t size)
{
    if (eos.init_end == 0) {
        return (eos_s8_t)EosRunErr_NotInitEnd;
    }

#if (EOS_USE_PUB_SUB != 0)
    if (eos.sub_table == EOS_NULL) {
        return (eos_s8_t)EosRunErr_SubTableNull;
    }
#endif

    // 保证框架已经运行
    if (eos.enabled == 0) {
        return (eos_s8_t)EosRun_NotEnabled;
    }

    if (size != 0) {
        return (eos_s8_t)EosRunErr_InvalidEventData;
    }

    if (eos.actor_exist == 0) {
        return (eos_s8_t)EosRun_NoActor;
    }

    // 没有状态机使能，返回
    if (eos.actor_enabled == 0) {
        return (eos_s8_t)EosRun_NotEnabled;
    }
    // 没有状态机订阅，返回
#if (EOS_USE_PUB_SUB != 0)
    if (eos.sub_table[topic] == 0) {
        return (eos_s8_t)EosRun_NoActorSub;
    }
#endif

    eos_port_critical_enter();
    // 申请事件空间
    eos_event_inner_t *e = eos_heap_malloc(&eos.heap, (size + sizeof(eos_event_inner_t)));
    if (e == (eos_event_inner_t *)0) {
        eos_port_critical_exit();
        return (eos_s8_t)EosRunErr_MallocFail;
    }
    e->topic = topic;
#if (EOS_USE_PUB_SUB != 0)
    e->sub = eos.sub_table[e->topic];
#else
    e->sub = eos.actor_exist;
#endif
    eos.heap.sub_general |= e->sub;
    eos_u8_t *e_data = (void *)(e + sizeof(eos_event_inner_t));
    for (eos_u32_t i = 0; i < size; i ++) {
        e_data[i] = ((eos_u8_t *)data)[i];
    }
    eos_port_critical_exit();

    return (eos_s8_t)EosRun_OK;
}

void eos_event_pub_topic(eos_topic_t topic)
{
    eos_s8_t ret = eos_event_pub_ret(topic, EOS_NULL, 0);
    EOS_ASSERT(ret >= 0);
    (void)ret;
}

#if (EOS_USE_EVENT_DATA != 0)
void eos_event_pub(eos_topic_t topic, void *data, eos_u32_t size)
{
    eos_s8_t ret = eos_event_pub_ret(topic, data, size);
    EOS_ASSERT(ret >= 0);
    (void)ret;
}
#endif

#if (EOS_USE_PUB_SUB != 0)
void eos_event_sub(eos_actor_t * const me, eos_topic_t topic)
{
    eos.sub_table[topic] |= (1 << me->priority);
}

void eos_event_unsub(eos_actor_t * const me, eos_topic_t topic)
{
    eos.sub_table[topic] &= ~(1 << me->priority);
}
#endif

#if (EOS_USE_TIME_EVENT != 0)
void eos_event_pub_time(eos_topic_t topic, eos_u32_t time_ms, eos_bool_t oneshoot)
{
    EOS_ASSERT(time_ms != 0);
    EOS_ASSERT(time_ms <= timer_threshold[EosTimerUnit_Minute]);
    EOS_ASSERT(eos.timer_count < EOS_MAX_TIME_EVENT);

    // 检查重复，不允许重复发送。
    for (eos_u32_t i = 0; i < eos.timer_count; i ++) {
        EOS_ASSERT(topic != eos.etimer[i].topic);
    }

    eos_u32_t system_ms = eos.time;
    eos_u8_t unit = EosTimerUnit_Ms;
    eos_u16_t period;
    for (eos_u8_t i = 0; i < EosTimerUnit_Max; i ++) {
        if (time_ms > timer_threshold[i])
            continue;
        unit = i;
        
        if (i == EosTimerUnit_Ms) {
            period = time_ms;
            break;
        }
        period =    (((time_ms / (timer_unit[i] / 10)) % 10) < 5 ?
                    (time_ms / timer_unit[i]) :
                    (time_ms / timer_unit[i] + 1));
        break;
    }
    eos_u32_t timeout = (system_ms + time_ms);
    eos.etimer[eos.timer_count ++] = (eos_event_timer_t) {
        topic, oneshoot, unit, period, timeout
    };
    
    if (eos.timeout_min > timeout) {
        eos.timeout_min = timeout;
    }
}

void eos_event_pub_delay(eos_topic_t topic, eos_u32_t time_ms)
{
    eos_event_pub_time(topic, time_ms, EOS_True);
}

void eos_event_pub_period(eos_topic_t topic, eos_u32_t time_ms_period)
{
    eos_event_pub_time(topic, time_ms_period, EOS_False);
}

void eos_event_time_cancel(eos_topic_t topic)
{
    eos_u32_t timeout_min = EOS_U32_MAX;
    for (eos_u32_t i = 0; i < eos.timer_count; i ++) {
        if (topic != eos.etimer[i].topic) {
            timeout_min = eos.etimer[i].timeout_ms;
            continue;
        }
        if (i == (eos.timer_count - 1)) {
            eos.timer_count --;
            break;
        }
        else {
            eos.etimer[i] = eos.etimer[eos.timer_count - 1];
            eos.timer_count -= 1;
            i --;
        }
    }

    eos.timeout_min = timeout_min;
}
#endif

// state tran ------------------------------------------------------------------
#if (EOS_USE_SM_MODE != 0)
eos_ret_t eos_tran(eos_sm_t * const me, eos_state_handler state)
{
    me->state = state;

    return EOS_Ret_Tran;
}

eos_ret_t eos_super(eos_sm_t * const me, eos_state_handler state)
{
    me->state = state;

    return EOS_Ret_Super;
}

eos_ret_t eos_state_top(eos_sm_t * const me, eos_event_t const * const e)
{
    (void)me;
    (void)e;

    return EOS_Ret_Null;
}
#endif

// static function -------------------------------------------------------------
#if (EOS_USE_SM_MODE != 0)
static void eos_sm_dispath(eos_sm_t * const me, eos_event_t const * const e)
{
#if (EOS_USE_HSM_MODE != 0)
    eos_state_handler path[EOS_MAX_HSM_NEST_DEPTH];
#endif
    eos_ret_t r;

    EOS_ASSERT(e != (eos_event_t *)0);

#if (EOS_USE_HSM_MODE == 0)
    eos_state_handler s = me->state;
    eos_state_handler t;
    
    r = s(me, e);
    if (r == EOS_Ret_Tran) {
        t = me->state;
        r = s(me, &eos_event_table[Event_Exit]);
        EOS_ASSERT(r == EOS_Ret_Handled || r == EOS_Ret_Super);
        r = t(me, &eos_event_table[Event_Enter]);
        EOS_ASSERT(r == EOS_Ret_Handled || r == EOS_Ret_Super);
        me->state = t;
    }
    else {
        me->state = s;
    }
#else
    eos_state_handler t = me->state;
    eos_state_handler s;

    // 层次化的处理事件
    // 注：分为两种情况：
    // (1) 当该状态存在数据时，处理此事件。
    // (2) 当该状态不存在该事件时，到StateTop状态下处理此事件。
    do {
        s = me->state;
        r = (*s)(me, e);                              // 执行状态S下的事件处理
    } while (r == EOS_Ret_Super);

    // 如果不存在状态转移
    if (r != EOS_Ret_Tran) {
        me->state = t;                                  // 更新当前状态
        return;
    }

    // 如果存在状态转移
    path[0] = me->state;    // 保存目标状态
    path[1] = t;
    path[2] = s;

    // exit current state to transition source s...
    while (t != s) {
        // exit handled?
        if (HSM_TRIG_(t, Event_Exit) == EOS_Ret_Handled) {
            (void)HSM_TRIG_(t, Event_Null); // find superstate of t
        }
        t = me->state; // stateTgt_ holds the superstate
    }

    eos_s32_t ip = eos_sm_tran(me, path); // take the HSM transition

    // retrace the entry path in reverse (desired) order...
    for (; ip >= 0; --ip) {
        HSM_TRIG_(path[ip], Event_Enter); // enter path[ip]
    }
    t = path[0];    // stick the target into register
    me->state = t; // update the next state

    // 一级一级的钻入各层
    while (HSM_TRIG_(t, Event_Init) == EOS_Ret_Tran) {
        ip = 0;
        path[0] = me->state;
        (void)HSM_TRIG_(me->state, Event_Null);       // 获取其父状态
        while (me->state != t) {
            ip ++;
            path[ip] = me->state;
            (void)HSM_TRIG_(me->state, Event_Null);   // 获取其父状态
        }
        me->state = path[0];

        // 层数不能大于MAX_NEST_DEPTH_
        EOS_ASSERT(ip < EOS_MAX_HSM_NEST_DEPTH);

        // retrace the entry path in reverse (correct) order...
        do {
            HSM_TRIG_(path[ip], Event_Enter);       // 进入path[ip]
        } while (ip >= 0);

        t = path[0];
    }

    me->state = t;                                  // 更新当前状态
#endif
}

#if (EOS_USE_HSM_MODE != 0)
static eos_s32_t eos_sm_tran(eos_sm_t * const me, eos_state_handler path[EOS_MAX_HSM_NEST_DEPTH])
{
    // transition entry path index
    eos_s32_t ip = -1;
    eos_s32_t iq; // helper transition entry path index
    eos_state_handler t = path[0];
    eos_state_handler s = path[2];
    eos_ret_t r;

    // (a) 跳转到自身 s == t
    if (s == t) {
        HSM_TRIG_(s, Event_Exit);  // exit the source
        return 0; // cause entering the target
    }

    (void)HSM_TRIG_(t, Event_Null); // superstate of target
    t = me->state;

    // (b) check source == target->super
    if (s == t)
        return 0; // cause entering the target

    (void)HSM_TRIG_(s, Event_Null); // superstate of src

    // (c) check source->super == target->super
    if (me->state == t) {
        HSM_TRIG_(s, Event_Exit);  // exit the source
        return 0; // cause entering the target
    }

    // (d) check source->super == target
    if (me->state == path[0]) {
        HSM_TRIG_(s, Event_Exit); // exit the source
        return -1;
    }

    // (e) check rest of source == target->super->super..
    // and store the entry path along the way

    // indicate that the LCA was not found
    iq = 0;

    // enter target and its superstate
    ip = 1;
    path[1] = t; // save the superstate of target
    t = me->state; // save source->super

    // find target->super->super
    r = HSM_TRIG_(path[1], Event_Null);
    while (r == EOS_Ret_Super) {
        ++ ip;
        path[ip] = me->state; // store the entry path
        if (me->state == s) { // is it the source?
            // indicate that the LCA was found
            iq = 1;

            // entry path must not overflow
            EOS_ASSERT(ip < EOS_MAX_HSM_NEST_DEPTH);
            --ip;  // do not enter the source
            r = EOS_Ret_Handled; // terminate the loop
        }
        // it is not the source, keep going up
        else
            r = HSM_TRIG_(me->state, Event_Null);
    }

    // LCA found yet?
    if (iq == 0) {
        // entry path must not overflow
        EOS_ASSERT(ip < EOS_MAX_HSM_NEST_DEPTH);

        HSM_TRIG_(s, Event_Exit); // exit the source

        // (f) check the rest of source->super
        //                  == target->super->super...
        iq = ip;
        r = EOS_Ret_Null; // indicate LCA NOT found
        do {
            // is this the LCA?
            if (t == path[iq]) {
                r = EOS_Ret_Handled; // indicate LCA found
                // do not enter LCA
                ip = iq - 1;
                // cause termination of the loop
                iq = -1;
            }
            else
                -- iq; // try lower superstate of target
        } while (iq >= 0);

        // LCA not found yet?
        if (r != EOS_Ret_Handled) {
            // (g) check each source->super->...
            // for each target->super...
            r = EOS_Ret_Null; // keep looping
            do {
                // exit t unhandled?
                if (HSM_TRIG_(t, Event_Exit) == EOS_Ret_Handled) {
                    (void)HSM_TRIG_(t, Event_Null);
                }
                t = me->state; //  set to super of t
                iq = ip;
                do {
                    // is this LCA?
                    if (t == path[iq]) {
                        // do not enter LCA
                        ip = iq - 1;
                        // break out of inner loop
                        iq = -1;
                        r = EOS_Ret_Handled; // break outer loop
                    }
                    else
                        --iq;
                } while (iq >= 0);
            } while (r != EOS_Ret_Handled);
        }
    }

    return ip;
}
#endif
#endif

/* heap library ------------------------------------------------------------- */
void eos_heap_init(eos_heap_t * const me)
{
    eos_block_t * block_1st;
    
    // block start
    me->queue = EOS_HEAP_MAX;
    me->error_id = 0;
    me->size = EOS_SIZE_HEAP;
    me->empty = 1;
    me->sub_general = 0;
    me->current = EOS_HEAP_MAX;

    memset(me->data, 0, EOS_SIZE_HEAP);

    // the 1st free block
    block_1st = (eos_block_t *)(me->data);
   
    block_1st->last = EOS_HEAP_MAX;
    block_1st->size = EOS_SIZE_HEAP - (eos_u16_t)sizeof(eos_block_t);
    block_1st->free = 1;
    block_1st->next = EOS_HEAP_MAX;
}

void * eos_heap_malloc(eos_heap_t * const me, eos_u32_t size)
{
    eos_block_t * block;
    eos_s16_t remaining;

    if (size == 0) {
        me->error_id = 1;
        return EOS_NULL;
    }

    /* Find the first free block in the block-list. */
    eos_u16_t next = 0;
    do {
        block = (eos_block_t *)(me->data + next);
        remaining = (block->size - size - sizeof(eos_block_t));
        if (block->free == 1 && remaining >= 0) {
            break;
        }
        next = block->next;
    } while (next != EOS_HEAP_MAX);

    if (next == EOS_HEAP_MAX) {
        me->error_id = 2;
        return EOS_NULL;
    }

    /* Divide the block into two blocks. */
    /* ARM Cortex-M0不支持非对齐访问 */
    size = (size % 4 == 0) ? size : (size + 4 - (size % 4));
    eos_pointer_t address = (eos_pointer_t)block + size + sizeof(eos_block_t);
    eos_block_t * new_block = (eos_block_t *)address;
    eos_u32_t _size = block->size - size - sizeof(eos_block_t);

    /* Update the list. */
    new_block->size = _size;
    new_block->free = 1;
    new_block->next = block->next;
    new_block->last = (eos_u16_t)((eos_pointer_t)block - (eos_pointer_t)me->data);
    block->next = (eos_u16_t)((eos_pointer_t)new_block - (eos_pointer_t)me->data);
    block->size = size;
    block->free = 0;

    if (new_block->next != EOS_HEAP_MAX) {
        eos_block_t * block_next2 = (eos_block_t *)((eos_pointer_t)me->data + new_block->next);
        block_next2->last = (eos_u16_t)((eos_pointer_t)new_block - (eos_pointer_t)me->data);
    }

    /* 挂在Queue的最后端 */
    next = me->queue;
    eos_block_t * block_queue;
    if (me->queue == EOS_HEAP_MAX) {
        me->queue = (eos_u16_t)((eos_pointer_t)block - (eos_pointer_t)me->data);
        block->q_next = EOS_HEAP_MAX;
        block->q_last = EOS_HEAP_MAX;
        me->current = me->queue;
    }
    else {
        do {
            block_queue = (eos_block_t *)(me->data + next);
            next = block_queue->q_next;
        } while (next != EOS_HEAP_MAX);

        block_queue->q_next = (eos_u16_t)((eos_pointer_t)block - (eos_pointer_t)me->data);
        block->q_next = EOS_HEAP_MAX;
        block->q_last = (eos_u16_t)((eos_pointer_t)block_queue - (eos_pointer_t)me->data);
    }

    me->error_id = 0;
    me->empty = 0;
    void *p = (void *)((eos_pointer_t)block + (eos_u32_t)sizeof(eos_block_t));
    me->count ++;

    return p;
}

void eos_heap_gc(eos_heap_t * const me, void *data)
{
    eos_event_inner_t *e = (eos_event_inner_t *)data;

    if (e->sub == 0) {
        eos_block_t *block = (eos_block_t *)((eos_pointer_t)data - sizeof(eos_block_t));
        eos_u16_t index = (eos_u16_t)((eos_pointer_t)block - (eos_pointer_t)me->data);
        eos_block_t *block_last = (eos_block_t *)(me->data + block->q_last);
        eos_block_t *block_next = (eos_block_t *)(me->data + block->q_next);

        /* 从Queue中删除 */
        // 如果当前只有这一个block
        if (block->q_next == EOS_HEAP_MAX && block->q_last == EOS_HEAP_MAX) {
            me->empty = 1;
            me->current = EOS_HEAP_MAX;
            me->queue = EOS_HEAP_MAX;
        }
        // 如果这个block在Queue的第一个
        else if (me->queue == index) {
            block_next->q_last = EOS_HEAP_MAX;
            me->queue = block->q_next;
            me->current = block->q_next;
        }
        // 如果这个block在Queue的最后一个
        else if (block->q_next == EOS_HEAP_MAX) {
            block_last->q_next = EOS_HEAP_MAX;
            me->current = me->queue;
        }
        else {
            block_last->q_next = block->q_next;
            block_next->q_last = block->q_last;
            me->current = block->q_next;
        }

        /* 释放这块内存 */
        eos_heap_free(me, data);
    }

    /* 根据所有的sub重新生成sub_general */
    me->sub_general = 0;
    eos_u16_t next = me->queue;
    eos_u16_t loop_count = 0;
    eos_block_t *block;
    while (next != EOS_HEAP_MAX && loop_count < me->count) {
        eos_event_inner_t *evt;
        block = (eos_block_t *)((eos_pointer_t)me->data + next);
        evt = (eos_event_inner_t *)((eos_pointer_t)block + sizeof(eos_block_t));
        me->sub_general |= evt->sub;
        next = block->q_next;
    }
}

void *eos_heap_get_block(eos_heap_t * const me, eos_u8_t priority)
{
    eos_block_t * block = EOS_NULL;
    eos_event_inner_t *e = EOS_NULL;

    EOS_ASSERT(priority < EOS_MAX_ACTORS);

    eos_u16_t next = me->current;
    eos_u16_t loop_count = 0;
    while (next != EOS_HEAP_MAX && loop_count < me->count) {
        eos_event_inner_t *evt;
        block = (eos_block_t *)((eos_pointer_t)me->data + next);
        EOS_ASSERT(block->free == 0);
        evt = (eos_event_inner_t *)((eos_pointer_t)block + sizeof(eos_block_t));
        if ((evt->sub & (1 << priority)) == 0) {
            next = block->q_next;
            loop_count ++;
        }
        else {
            e = evt;
            evt->sub &=~ (1 << priority);
            break;
        }
    }

    return (void *)e;
}

void eos_heap_free(eos_heap_t * const me, void * data)
{
    eos_block_t * block = (eos_block_t *)((eos_pointer_t)data - sizeof(eos_block_t));
    eos_block_t * block_next;
    me->error_id = 0;
    if (block->last != EOS_HEAP_MAX) {
        eos_block_t * block_last = (eos_block_t *)(me->data + block->last);
        /* Check the block can be combined with the front one. */
        if (block_last->free == 1) {
            block_last->next = block->next;
            if (block->next != EOS_HEAP_MAX) {
                block_next = (eos_block_t *)(me->data + block_last->next);
                block_next->last = (eos_u16_t)((eos_pointer_t)block_last - (eos_pointer_t)me->data);
            }
            block_last->size += (block->size + sizeof(eos_block_t));
            block = block_last;
        }
    }
    
    /* Check the block can be combined with the later one. */
    if (block->next != EOS_HEAP_MAX) {
        eos_block_t * block_next = (eos_block_t *)(me->data + block->next);
        eos_block_t * block_next2;
        if (block_next->free == 1) {
            block->size += (block_next->size + (eos_u32_t)sizeof(eos_block_t));
            block->next = block_next->next;
            if (block->next != EOS_HEAP_MAX) {
                block_next2 = (eos_block_t *)(me->data + block_next->next);
                block_next2->last = (eos_u16_t)((eos_pointer_t)block - (eos_pointer_t)me->data);
            }
        }
    }

    block->free = 1;
    me->count --;
}

/* for unittest ------------------------------------------------------------- */
void * eos_get_framework(void)
{
    return (void *)&eos;
}

#if (EOS_USE_TIME_EVENT != 0)
void eos_set_time(eos_u32_t time_ms)
{
    eos.time = time_ms;
}
#endif

#ifdef __cplusplus
}
#endif
