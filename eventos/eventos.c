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

// include ---------------------------------------------------------------------
#include "eventos.h"
#if (EOS_USE_EVENT_DATA != 0 && EOS_USE_HEAP_LOCAL == 0)
#include <stdlib.h>
#endif

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

#define EOS_ASSERT_ID(id_, test_) do { if (!(test_)) {                         \
        eos_hook_stop();                                                       \
        eos_port_critical_enter();                                             \
        eos_port_assert(id_);                                                  \
    } } while (0);
#else
#define EOS_ASSERT(test_)               ((void)0)
#define EOS_ASSERT_ID(id_, test_)       ((void)0)
#endif

// eos define ------------------------------------------------------------------
enum eos_actor_mode {
    EOS_Mode_Reactor = 0,
    EOS_Mode_StateMachine = !EOS_Mode_Reactor
};

// **eos** ---------------------------------------------------------------------
#if (EOS_USE_TIME_EVENT != 0)
typedef struct eos_event_timer {
    eos_topic_t topic;
    eos_bool_t is_one_shoot;
    eos_s32_t time_ms_delay;
    eos_u32_t timeout_ms;
} eos_event_timer_t;
#endif

typedef struct eos_block {
    struct eos_block *next;
    eos_u8_t free;
    eos_u16_t size;
} eos_block_t;

typedef struct eos_heap {
    eos_u8_t data[EOS_SIZE_HEAP];
    eos_block_t *list;
    eos_u16_t size;       /* total size */
    eos_u8_t error_id;
} eos_heap_t;

typedef struct eos_event_inner {
    eos_u32_t topic;
    void *data;
    eos_u32_t flag_sub;
} eos_event_inner_t;

typedef struct eos_tag {
    eos_mcu_t magic;
#if (EOS_USE_PUB_SUB != 0)
    eos_mcu_t *sub_table;                                     // event sub table
#endif

    eos_mcu_t actor_exist;
    eos_mcu_t sm_enabled;
    eos_actor_t * actor[EOS_MAX_ACTORS];

#if (EOS_USE_EVENT_DATA != 0)
    eos_heap_t heap;
#endif

#if (EOS_USE_TIME_EVENT != 0)
    eos_event_timer_t e_timer_pool[EOS_MAX_TIME_EVENT];
    eos_u32_t flag_etimerpool[EOS_MAX_TIME_EVENT / 32 + 1];    // timer pool flag
    eos_u32_t timeout_ms_min;
    eos_u32_t time_crt_ms;
    eos_bool_t etimerpool_empty;
#endif

    eos_bool_t enabled;
    eos_bool_t running;
    eos_bool_t idle;
} eos_t;
// **eos end** -----------------------------------------------------------------

eos_t eos;

#if (EOS_MCU_TYPE == 8)
#define EOS_MAGIC                         0x4F
#elif (EOS_MCU_TYPE == 16)
#define EOS_MAGIC                         0x4F2E
#else
#define EOS_MAGIC                         0x4F2EA035
#endif

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

#define HSM_EXIT_(state_) do { HSM_TRIG_(state_, Event_Exit); } while (0)
#define HSM_ENTER_(state_) do { HSM_TRIG_(state_, Event_Enter); } while (0)
#endif

// static function -------------------------------------------------------------
#if (EOS_USE_SM_MODE != 0)
static void eos_sm_dispath(eos_sm_t * const me, eos_event_t const * const e);
#if (EOS_USE_HSM_MODE != 0)
static eos_s32_t eos_sm_tran(eos_sm_t * const me, eos_state_handler path[EOS_MAX_HSM_NEST_DEPTH]);
#endif
#endif
#if (EOS_USE_EVENT_DATA != 0 && EOS_USE_HEAP_LOCAL != 0)
void eos_heap_init(eos_heap_t * const me);
void * eos_heap_malloc(eos_heap_t * const me, eos_u32_t size);
void eos_heap_free(eos_heap_t * const me, void * data);
#endif

// eventos ---------------------------------------------------------------------
static void eos_clear(void)
{
#if (EOS_USE_TIME_EVENT != 0)
    // Clear all time-events' pool.
    for (eos_u32_t i = 0; i < EOS_MAX_TIME_EVENT / 32 + 1; i ++)
        eos.flag_etimerpool[i] = EOS_U32_MAX;
    for (eos_u32_t i = 0; i < EOS_MAX_TIME_EVENT; i ++)
        eos.flag_etimerpool[i / 32] &= ~(1 << (i % 32));
    eos.etimerpool_empty = EOS_True;
#endif
}

void eventos_init(void)
{
    eos_clear();

    eos.enabled = EOS_True;
    eos.running = EOS_False;
    eos.idle = EOS_True;
    eos.magic = EOS_MAGIC;
#if (EOS_USE_PUB_SUB != 0)
    eos.sub_table = 0;
#endif

#if (EOS_USE_EVENT_DATA != 0 && EOS_USE_HEAP_LOCAL != 0)
    eos_heap_init(&eos.heap);
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
    eos.time_crt_ms = eos_port_get_time_ms();
    
    if (eos.etimerpool_empty == EOS_True)
        return 210;

    // 时间未到达
    if (eos.time_crt_ms < eos.timeout_ms_min)
        return 211;
    
    // 若时间到达，将此事件推入事件队列，同时在e_timer_pool里删除。
    eos_bool_t etimerpool_empty = EOS_True;
    for (eos_u32_t i = 0; i < EOS_MAX_TIME_EVENT; i ++) {
        if ((eos.flag_etimerpool[i / 32] & (1 << (i % 32))) == 0)
            continue;
        if (eos.e_timer_pool[i].timeout_ms > eos.time_crt_ms) {
            etimerpool_empty = EOS_False;
            continue;
        }
        eos_event_pub_topic(eos.e_timer_pool[i].topic);
        // 清零标志位
        if (eos.e_timer_pool[i].is_one_shoot == EOS_True)
            eos.flag_etimerpool[i / 32] &= ~(1 << (i % 32));
        else {
            eos.e_timer_pool[i].timeout_ms += eos.e_timer_pool[i].time_ms_delay;
            etimerpool_empty = EOS_False;
        }
    }
    eos.etimerpool_empty = etimerpool_empty;
    if (eos.etimerpool_empty == EOS_True)
        return 212;

    // 寻找到最小的时间定时器
    eos_u32_t min_time_out_ms = EOS_U32_MAX;
    for (eos_u32_t i = 0; i < EOS_MAX_TIME_EVENT; i ++) {
        if ((eos.flag_etimerpool[i / 32] & (1 << (i % 32))) == 0)
            continue;
        if (min_time_out_ms <= eos.e_timer_pool[i].timeout_ms)
            continue;
        min_time_out_ms = eos.e_timer_pool[i].timeout_ms;
    }
    eos.timeout_ms_min = min_time_out_ms;

    return 0;
}
#endif

eos_s32_t eos_once(void)
{
    eos_s32_t ret = 0;

    if (eos.enabled == EOS_False) {
        eos_clear();
        return 1;
    }

    // 检查是否有状态机的注册
    if (eos.actor_exist == 0 || eos.sm_enabled == 0) {
        ret = 201;
        return ret;
    }

#if (EOS_USE_TIME_EVENT != 0)
    eos_evttimer();
#endif

    if (eos.idle == EOS_True) {
        ret = 202;
        return ret;
    }
    // 寻找到优先级最高且事件队列不为空的状态机
    eos_actor_t *actor = (eos_actor_t *)0;
    for (eos_u32_t i = 0; i < EOS_MAX_ACTORS; i ++) {
        if ((eos.actor_exist & (1 << i)) == 0)
            continue;
        EOS_ASSERT(eos.actor[i]->magic == EOS_MAGIC);
        if (eos.actor[i]->equeue_empty == EOS_True)
            continue;
        actor = eos.actor[i];
        break;
    }
    if (actor == (eos_actor_t *)0) {
        eos.idle = EOS_True;
        
        ret = 203;
        return ret;
    }

    // 寻找到最老的事件
    eos_port_critical_enter();
    eos_event_inner_t * _e = ((eos_event_inner_t **)actor->e_queue)[actor->tail];
    actor->tail ++;
    actor->tail %= actor->depth;
    if (actor->tail == actor->head) {
        actor->equeue_empty = EOS_True;
        actor->tail = 0;
        actor->head = 0;
    }

    _e->flag_sub &= ~(1 << (actor->priority));
    eos_port_critical_exit();
    // 对事件进行执行
#if (EOS_USE_PUB_SUB != 0)
    if ((eos.sub_table[_e->topic] & (1 << actor->priority)) != 0)
#endif
    {
#if (EOS_USE_SM_MODE != 0)
        if (actor->mode == EOS_Mode_StateMachine) {
            // 执行状态的转换
            eos_sm_t *sm = (eos_sm_t *)actor;
            eos_sm_dispath(sm, (eos_event_t *)_e);
        }
        else 
#endif
        {
            eos_reactor_t *reactor = (eos_reactor_t *)actor;
            reactor->event_handler(reactor, (eos_event_t *)_e);
        }
    }
#if (EOS_USE_PUB_SUB != 0)
    else
        ret = 204;
#endif
#if (EOS_USE_EVENT_DATA != 0)
    // 销毁过期事件与其携带的参数
    if (_e->flag_sub == 0) {
        eos_port_critical_enter();
#if (EOS_USE_HEAP_LOCAL == 0)
        free(_e);
#else
        eos_heap_free(&eos.heap, _e);
#endif
        eos_port_critical_exit();
    }
#endif

    return ret;
}

void eventos_run(void)
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
        // 检查Magic
        EOS_ASSERT(eos.magic == EOS_MAGIC);
        eos_s32_t ret = eos_once();
        if ((int)(ret / 100) == 2) {
            eos_hook_idle();
        }
        else if (ret == 1)
            break;
    }

    while (1) {
        eos_hook_idle();
    }
}

void eventos_stop(void)
{
    eos.enabled = EOS_False;
    eos_hook_stop();
}

// 关于Reactor -----------------------------------------------------------------
static void eos_actor_init( eos_actor_t * const me,
                            eos_u32_t priority,
                            eos_event_quote_t *event_queue, eos_u32_t queue_size)
{
    // 框架需要先启动起来
    EOS_ASSERT(eos.enabled == EOS_True);
    EOS_ASSERT(eos.running == EOS_False);
    // 参数检查
    EOS_ASSERT(me != (eos_actor_t *)0);
    EOS_ASSERT(priority < EOS_MAX_ACTORS);

    me->magic = EOS_MAGIC;

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

    // 事件队列
    eos_port_critical_enter();
    me->e_queue = event_queue;
    EOS_ASSERT_ID(105, me->e_queue != 0);
    me->head = 0;
    me->tail = 0;
    me->depth = queue_size * 4;
    me->equeue_empty = EOS_True;
    eos_port_critical_exit();
}

void eos_reactor_init(  eos_reactor_t * const me,
                        eos_u32_t priority,
                        eos_event_quote_t *event_queue, eos_u32_t queue_size)
{
    eos_actor_init(&me->super, priority, event_queue, queue_size);
    me->super.mode = EOS_Mode_Reactor;
}

void eos_reactor_start(eos_reactor_t * const me, eos_event_handler event_handler)
{
    me->event_handler = event_handler;
    me->super.enabled = EOS_True;
    eos.sm_enabled |= (1 << me->super.priority);
}

// state machine ---------------------------------------------------------------
#if (EOS_USE_SM_MODE != 0)
void eos_sm_init(   eos_sm_t * const me,
                    eos_u32_t priority,
                    eos_event_quote_t *event_queue, eos_u32_t queue_size)
{
    eos_actor_init(&me->super, priority, event_queue, queue_size);
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
    eos.sm_enabled |= (1 << me->super.priority);

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
            HSM_ENTER_(path[ip --]);
        } while (ip >= 0);

        t = path[0];

        ret = HSM_TRIG_(t, Event_Init);
    } while (ret == EOS_Ret_Tran);

    me->state = t;
#endif
}
#endif

// event -----------------------------------------------------------------------
void eos_event_pub_topic(eos_topic_t topic)
{
    eos_u8_t para;
    eos_event_pub(topic, &para, 1);
}

int count = 0;
#if (EOS_USE_EVENT_DATA != 0)
void eos_event_pub(eos_topic_t topic, void *data, eos_u32_t size)
{
    // 保证框架已经运行
    EOS_ASSERT(eos.enabled == EOS_True);
    // 没有状态机使能，返回
    if (eos.sm_enabled == 0) {
        return;
    }
    // 没有状态机订阅，返回
#if (EOS_USE_PUB_SUB != 0)
    if (eos.sub_table[topic] == 0) {
#else
    if (eos.actor_exist == 0) {
#endif
        return;
    }
    eos_port_critical_enter();
    // 申请事件空间
#if (EOS_USE_HEAP_LOCAL == 0)
    eos_event_inner_t *e = malloc(size + sizeof(eos_event_inner_t));
#else
    eos_event_inner_t *e = eos_heap_malloc(&eos.heap, (size + sizeof(eos_event_inner_t)));
#endif
    EOS_ASSERT(e != (eos_event_inner_t *)0);
    e->topic = topic;
#if (EOS_USE_PUB_SUB != 0)
    e->flag_sub = eos.sub_table[e->topic];
#else
    e->flag_sub = eos.actor_exist;
#endif
    e->data = (void *)(e + sizeof(eos_event_inner_t));
    for (eos_u32_t i = 0; i < size; i ++) {
        ((eos_u8_t *)e->data)[i] = ((eos_u8_t *)data)[i];
    }
    // 根据flagSub的信息，将事件推入各对象
    for (eos_u32_t i = 0; i < EOS_MAX_ACTORS; i ++) {
        if ((eos.actor_exist & (1 << i)) == 0) {
            continue;
        }
        if (eos.actor[i]->enabled == EOS_False) {
            continue;
        }
#if (EOS_USE_PUB_SUB != 0)
        if ((eos.sub_table[e->topic] & (1 << i)) == 0) {
#else
        if ((eos.actor_exist && (1 << i)) == 0) {
#endif
            continue;
        }
        // 如果事件队列满，进入断言
        if (((eos.actor[i]->head + 1) % eos.actor[i]->depth) == eos.actor[i]->tail)
            EOS_ASSERT_ID(104, EOS_False);
        // 事件队列未满，将事件池序号放入事件队列
        ((eos_event_inner_t **)eos.actor[i]->e_queue)[eos.actor[i]->head] = e;
        
        eos.actor[i]->head ++;
        eos.actor[i]->head %= eos.actor[i]->depth;
        if (eos.actor[i]->equeue_empty == EOS_True)
            eos.actor[i]->equeue_empty = EOS_False;
        eos.idle = EOS_False;
    }
    eos_port_critical_exit();
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
static void evt_publish_time(eos_s32_t topic, eos_s32_t time_ms, eos_bool_t is_oneshoot)
{
    EOS_ASSERT(time_ms >= 0);
    EOS_ASSERT(!(time_ms == 0 && is_oneshoot == EOS_False));

    if (time_ms == 0) {
        eos_event_pub_topic(topic);
        return;
    }

    // 如果是周期性的，检查是否已经对某个事件进行过周期设定。
    if (is_oneshoot == EOS_False && eos.etimerpool_empty == EOS_False) {
        eos_bool_t is_topic_set = EOS_False;
        for (eos_u32_t i = 0; i < EOS_MAX_TIME_EVENT; i ++) {
            if ((eos.flag_etimerpool[i / 32] & (1 << (i % 32))) == 0)
                continue;
            if (eos.e_timer_pool[i].topic != topic)
                continue;
            if (eos.e_timer_pool[i].time_ms_delay == time_ms)
                return;
            is_topic_set = EOS_True;
            break;
        }
        EOS_ASSERT_ID(106, is_topic_set == EOS_False);
    }

    // Find an empty soft timer.
    eos_s32_t index_empty = EOS_U32_MAX;
    for (eos_u32_t i = 0; i < (EOS_MAX_TIME_EVENT / 32 + 1); i ++) {
        if (eos.flag_etimerpool[i] == EOS_U32_MAX)
            continue;
        for (eos_s32_t j = 0; j < 32; j ++) {
            if ((eos.flag_etimerpool[i] & (1 << j)) == 0) {
                eos.flag_etimerpool[i] |= (1 << j);
                index_empty = i * 32 + j;
                break;
            }
        }
        break;
    }
    EOS_ASSERT_ID(102, index_empty != EOS_U32_MAX);

    eos_u32_t time_crt_ms = eos_port_get_time_ms();
    eos.e_timer_pool[index_empty] = (eos_event_timer_t) {
        topic, is_oneshoot, time_ms, (time_crt_ms + time_ms)
    };
    eos.etimerpool_empty = EOS_False;
    
    // Find the nearest soft timer.
    eos_u32_t min_time_out_ms = EOS_U32_MAX;
    for (eos_u32_t i = 0; i < EOS_MAX_TIME_EVENT; i ++) {
        if ((eos.flag_etimerpool[i / 32] & (1 << (i % 32))) == 0)
            continue;
        if (min_time_out_ms <= eos.e_timer_pool[i].timeout_ms)
            continue;
        min_time_out_ms = eos.e_timer_pool[i].timeout_ms;
    }
    eos.timeout_ms_min = min_time_out_ms;
}

void eos_event_pub_delay(eos_topic_t topic, eos_u32_t time_ms)
{
    evt_publish_time(topic, time_ms, EOS_True);
}

void eos_event_pub_period(eos_topic_t topic, eos_u32_t time_ms_period)
{
    evt_publish_time(topic, time_ms_period, EOS_False);
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
    eos_state_handler s = me->state;
    eos_state_handler t;
    eos_ret_t r;

    EOS_ASSERT(e != (eos_event_t *)0);

#if (EOS_USE_HSM_MODE == 0)
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
        HSM_ENTER_(path[ip]); // enter path[ip]
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
            HSM_ENTER_(path[ip --]);                   // 进入path[ip]
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
        HSM_EXIT_(s);  // exit the source
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
        HSM_EXIT_(s);  // exit the source
        return 0; // cause entering the target
    }

    // (d) check source->super == target
    if (me->state == path[0]) {
        HSM_EXIT_(s); // exit the source
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

        HSM_EXIT_(s); // exit the source

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

#if (EOS_USE_HEAP_LOCAL != 0)
/* heap library ------------------------------------------------------------- */
void eos_heap_init(eos_heap_t * const me)
{
    eos_block_t * block_1st;
    
    // block start
    me->list = (eos_block_t *)me->data;
    
    // the 1st free block
    block_1st = (eos_block_t *)me->data;;
    block_1st->next = (void *)0;
    block_1st->size = EOS_SIZE_HEAP - (eos_u32_t)sizeof(eos_block_t);
    block_1st->free = 1;

    me->error_id = 0;
    me->size = EOS_SIZE_HEAP;
}

void * eos_heap_malloc(eos_heap_t * const me, eos_u32_t size)
{
    eos_block_t * block;
    
    if (size == 0) {
        me->error_id = 1;
        return (void *)0;
    }

    /* Find the first free block in the block-list. */
    block = (eos_block_t *)me->list;
    do {
        if (block->free == 1 && block->size > (size + sizeof(eos_block_t))) {
            break;
        }
        else {
            block = block->next;
        }
    } while (block != (void *)0);

    if (block == (void *)0) {
        me->error_id = 2;
        return (void *)0;
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
    block->next = new_block;
    block->size = size;
    block->free = 0;

    me->error_id = 0;

    void *p = (void *)((eos_pointer_t)block + (eos_u32_t)sizeof(eos_block_t));

    return p;
}

void eos_heap_free(eos_heap_t * const me, void * data)
{
    eos_block_t * block_crt = (eos_block_t *)((eos_pointer_t)data - sizeof(eos_block_t));

    /* Search for this block in the block-list. */
    eos_block_t * block = me->list;
    eos_block_t * block_last = (void *)0;
    do {
        if (block->free == 0 && block == block_crt) {
            break;
        }
        else {
            block_last = block;
            block = block->next;
        }
    } while (block != (void *)0);

    /* Not found. */
    if (block == (void *)0) {
        me->error_id = 4;
        return;
    }

    block->free = 1;
    /* Check the block can be combined with the front one. */
    if (block_last != (eos_block_t *)0 && block_last->free == 1) {
        block_last->size += (block->size + sizeof(eos_block_t));
        block_last->next = block->next;
        block = block_last;
    }
    /* Check the block can be combined with the later one. */
    if (block->next != (eos_block_t *)0 && block->next->free == 1) {
        block->size += (block->next->size + (eos_u32_t)sizeof(eos_block_t));
        block->next = block->next->next;
        block->free = 1;
    }

    me->error_id = 0;
}
#endif

/* for unittest ------------------------------------------------------------- */
void * eos_get_framework(void)
{
    return (void *)&eos;
}

#ifdef __cplusplus
}
#endif
