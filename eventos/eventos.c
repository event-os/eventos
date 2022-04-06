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
 * https://github.com/event-os/eventos
 * https://gitee.com/event-os/eventos
 * 
 * Change Logs:
 * Date           Author        Notes
 * 2021-11-23     DogMing       V0.0.2
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
        eos_critical_enter();                                                  \
        eos_port_assert(__LINE__);                                             \
    } } while (0)
#else
#define EOS_ASSERT(test_)               ((void)0)
#endif

// eos define ------------------------------------------------------------------
enum eos_object_type {
    EosObj_Task = 0,
    EosObj_Reactor,
    EosObj_StateMachine,
    EosObj_Event,
    EosObj_Timer,
    EosObj_Device,
    EosObj_Heap,
    EosObj_Other,
};

/* eos task ----------------------------------------------------------------- */
eos_task_t *volatile eos_current;
eos_task_t *volatile eos_next;

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

    EosRet_Max,

    EosRunErr_NotInitEnd                    = -1,
    EosRunErr_ActorNotSub                   = -2,
    EosRunErr_MallocFail                    = -3,
    EosRunErr_SubTableNull                  = -4,
    EosRunErr_InvalidEventData              = -5,
    EosRunErr_HeapMemoryNotEnough           = -6,
    EosRunErr_TimerRepeated                 = -7,
};

typedef uint32_t (* hash_algorithm_t)(const char *string);

#if (EOS_USE_TIME_EVENT != 0)
#define EOS_MS_NUM_30DAY                    (2592000000)

enum {
    EosTimerUnit_Ms                         = 0,    // 60S, ms
    EosTimerUnit_100Ms,                             // 100Min, 50ms
    EosTimerUnit_Sec,                               // 16h, 500ms
    EosTimerUnit_Minute,                            // 15day, 30S

    EosTimerUnit_Max
};

static const uint32_t timer_threshold[EosTimerUnit_Max] = {
    60000,                                          // 60 S
    6000000,                                        // 100 Minutes
    57600000,                                       // 16 hours
    1296000000,                                     // 15 days
};

static const uint32_t timer_unit[EosTimerUnit_Max] = {
    1, 100, 1000, 60000
};

typedef struct eos_event_timer {
    const char *topic;
    uint32_t oneshoot                      : 1;
    uint32_t unit                          : 2;
    uint32_t period                        : 16;
    uint32_t timeout_ms;
} eos_event_timer_t;
#endif

typedef struct eos_block {
    // word[0]
    uint32_t next                          : 15;
    uint32_t q_next                        : 15;
    // word[1]
    uint32_t last                          : 15;
    uint32_t q_last                        : 15;
    uint32_t free                          : 1;
    // word[2]
    uint16_t size                          : 15;
    uint32_t offset                        : 8;
} eos_block_t;

typedef struct eos_event_inner {
    uint32_t sub;
    const char *topic;
} eos_event_inner_t;

typedef struct eos_heap {
    uint8_t data[EOS_SIZE_HEAP];
    // word[0]
    uint32_t size                          : 15;       /* total size */
    uint32_t queue                         : 15;
    uint32_t error_id                      : 2;
    // word[2]
    uint32_t current                       : 15;
    uint32_t empty                         : 1;
    // word[2]
    uint64_t sub_general;
    uint32_t count;
} eos_heap_t;

typedef union eos_obj_block {
    uint32_t event_sub;
    eos_task_t *task;
    eos_heap_t *heap;
    eos_timer_t *timer;
    eos_device_t *device;
    void *other;
} eos_obj_block_t;

typedef struct eos_object {
    const char *key;                                    // Key
    eos_obj_block_t block;                              // object block
    uint32_t type;                                      // Object type
} eos_object_t;

typedef struct eos_hash_table {
    eos_object_t object[EOS_MAX_OBJECTS];
    uint32_t prime_max;                                 // prime max
    uint32_t size;
} eos_hash_table_t;

typedef struct eos_tag {
    eos_hash_table_t hash;
    hash_algorithm_t hash_func;
    uint64_t task_exist;
    uint64_t actor_enabled;

    eos_object_t *task[EOS_MAX_TASKS];
    eos_timer_t *timers;
    uint32_t timer_out_min;

#if (EOS_USE_EVENT_DATA != 0)
    eos_heap_t heap;
#endif

#if (EOS_USE_TIME_EVENT != 0)
    eos_event_timer_t etimer[EOS_MAX_TIME_EVENT];
    uint32_t time;
    uint32_t timeout_min;
    uint8_t timer_count;
#endif
    uint32_t delay;
    
    uint16_t prime_max;

    uint8_t enabled                        : 1;
    uint8_t running                        : 1;
    uint8_t init_end                       : 1;
} eos_t;

/* eventos API for test ----------------------------- */
int8_t eos_execute(uint8_t priority);
int8_t eos_event_pub_ret(const char *topic, void *data, uint32_t size);
void * eos_get_framework(void);
void eos_event_pub_time(const char *topic, uint32_t time_ms, eos_bool_t oneshoot);
void eos_set_time(uint32_t time_ms);
void eos_set_hash(hash_algorithm_t hash);
// **eos end** -----------------------------------------------------------------

eos_t eos;

// data ------------------------------------------------------------------------
#if (EOS_USE_SM_MODE != 0)
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

static const eos_event_t eos_event_table[Event_User] = {
    {"Event_Null", (void *)0, 0},
    {"Event_Enter", (void *)0, 0},
    {"Event_Exit", (void *)0, 0},
#if (EOS_USE_HSM_MODE != 0)
    {"Event_Init", (void *)0, 0},
#endif
};
#endif

// macro -----------------------------------------------------------------------
#if (EOS_USE_SM_MODE != 0)
#define HSM_TRIG_(state_, topic_)                                              \
    ((*(state_))(me, &eos_event_table[topic_]))
#endif

#define LOG2(x) (32U - __builtin_clz(x))

// static function -------------------------------------------------------------
void eos_task_start_private(eos_task_t * const me,
                            eos_func_t func,
                            uint8_t priority,
                            void *stack_addr,
                            uint32_t stack_size);
static uint16_t eos_task_init(  eos_task_t * const me,
                                const char *name,
                                uint8_t priority,
                                void *stack, uint32_t size);
static void eos_sheduler(void);
static int8_t eos_get_current(void);
static uint32_t eos_hash_time33(const char *string);
static uint16_t eos_hash_insert(const char *string);
static uint16_t eos_hash_get_index(const char *string);
#if (EOS_USE_SM_MODE != 0)
static void eos_sm_dispath(eos_sm_t * const me, eos_event_t const * const e);
#if (EOS_USE_HSM_MODE != 0)
static int32_t eos_sm_tran(eos_sm_t * const me, eos_state_handler path[EOS_MAX_HSM_NEST_DEPTH]);
#endif
#endif
#if (EOS_USE_EVENT_DATA != 0)
void eos_heap_init(eos_heap_t * const me);
void * eos_heap_malloc(eos_heap_t * const me, uint32_t size);
void eos_heap_free(eos_heap_t * const me, void * data);
void *eos_heap_get_block(eos_heap_t * const me, uint8_t priority);
void eos_heap_gc(eos_heap_t * const me, void *data);
#endif

/* -----------------------------------------------------------------------------
EventOS
----------------------------------------------------------------------------- */
uint64_t stack_idle[32];
eos_task_t task_idle;

static void task_func_idle(void *parameter)
{
    (void)parameter;

    while (1) {
        eos_hook_idle();
    }
}

void eos_init(void)
{
#if (EOS_USE_TIME_EVENT != 0)
    eos.timer_count = 0;
#endif

    eos.enabled = EOS_True;
    eos.running = EOS_False;
    eos.task_exist = 0;
    eos.actor_enabled = 0;
    eos.hash_func = eos_hash_time33;

#if (EOS_USE_EVENT_DATA != 0)
    eos_heap_init(&eos.heap);
#endif

    eos.init_end = 1;
#if (EOS_USE_TIME_EVENT != 0)
    eos.time = 0;
#endif

    // 求最大素数
    for (int32_t i = EOS_MAX_OBJECTS; i > 0; i --) {
        bool is_prime = true;
        for (uint32_t j = 2; j < EOS_MAX_OBJECTS; j ++) {
            if (i <= j) {
                break;
            }
            if ((i % j) == 0) {
                is_prime = false;
                break;
            }
        }
        if (is_prime == false) {
            continue;
        }
        else {
            eos.prime_max = i;
            break;
        }
    }
    // 哈希表初始化
    for (uint32_t i = 0; i < EOS_MAX_OBJECTS; i ++) {
        eos.hash.object[i].key = (const char *)0;
    }
    
    eos_current = 0;
    eos_next = &task_idle;
    
    eos_task_start( &task_idle,
                    "task_idle",
                    task_func_idle, 0, stack_idle, sizeof(stack_idle));
}

/* -----------------------------------------------------------------------------
Timer
----------------------------------------------------------------------------- */
// 启动软定时器，允许在中断中调用。
void eos_timer_start(   eos_timer_t * const me,
                        const char *name,
                        uint32_t time_ms,
                        bool oneshoot,
                        eos_func_t callback)
{
    EOS_ASSERT(time_ms <= EOS_MS_NUM_30DAY);

    // 检查重名
    eos_critical_enter();
    uint16_t index = eos_hash_get_index(name);
    EOS_ASSERT(index == EOS_MAX_OBJECTS);
    eos_critical_exit();

    // Timer data.
    me->time = time_ms;
    me->time_out = eos.time + time_ms;
    me->callback = callback;
    me->oneshoot = oneshoot == false ? 0 : 1;
    me->running = 1;

    eos_critical_enter();
    // Add in the hash table.
    index = eos_hash_insert(name);
    eos.hash.object[index].type = EosObj_Timer;
    // Add the timer to the list.
    me->next = eos.timers;
    eos.timers = me;
    
    if (eos.timer_out_min > me->time_out) {
        eos.timer_out_min = me->time_out;
    }
    eos_critical_exit();
}

// 删除软定时器，允许在中断中调用。
void eos_timer_delete(const char *name)
{
    // 检查是否存在
    eos_critical_enter();
    uint16_t index = eos_hash_get_index(name);
    EOS_ASSERT(index != EOS_MAX_OBJECTS);

    eos_timer_t *current = eos.hash.object[index].block.timer;
    eos.hash.object[index].key = (const char *)0;
    eos_timer_t *list = eos.timers;
    eos_timer_t *last = (eos_timer_t *)0;
    while (list != (eos_timer_t *)0) {
        if (list == current) {
            if (last == (eos_timer_t *)0) {
                eos.timers = list->next;
            }
            else {
                last->next = list->next;
            }
            eos_critical_exit();
            return;
        }
        last = list;
        list = list->next;
    }

    // not found.
    EOS_ASSERT(0);
}

// 暂停软定时器，允许在中断中调用。
void eos_timer_pause(const char *name)
{
    // 检查是否存在
    eos_critical_enter();
    uint16_t index = eos_hash_get_index(name);
    EOS_ASSERT(index != EOS_MAX_OBJECTS);
    eos_timer_t *timer = eos.hash.object[index].block.timer;
    timer->running = 0;

    // 重新计算最小定时器值
    eos_timer_t *list = eos.timers;
    uint32_t time_out_min = UINT32_MAX;
    while (list != (eos_timer_t *)0) {
        if (list->running != 0 && time_out_min > list->time_out) {
            time_out_min = list->time_out;
        }
        list = list->next;
    }
    eos.timer_out_min = time_out_min;
    eos_critical_exit();
}

// 继续软定时器，允许在中断中调用。
void eos_timer_continue(const char *name)
{
    eos_critical_enter();
    uint16_t index = eos_hash_get_index(name);
    EOS_ASSERT(index != EOS_MAX_OBJECTS);
    eos_timer_t *timer = eos.hash.object[index].block.timer;
    timer->running = 1;
    if (eos.timer_out_min > timer->time_out) {
        eos.timer_out_min = timer->time_out;
    }
    eos_critical_exit();
}

// 重启软定时器的定时，允许在中断中调用。
void eos_timer_reset(const char *name)
{
    eos_critical_enter();
    uint16_t index = eos_hash_get_index(name);
    EOS_ASSERT(index != EOS_MAX_OBJECTS);
    eos_timer_t *timer = eos.hash.object[index].block.timer;
    timer->running = 1;
    timer->time_out = eos.time + timer->time;
    eos_critical_exit();
}

void eos_set_hash(hash_algorithm_t hash)
{
    eos.hash_func = hash;
}

#if (EOS_USE_TIME_EVENT != 0)
int32_t eos_evttimer(void)
{
    // 获取当前时间，检查延时事件队列
    uint32_t system_time = eos.time;
    
    if (eos.etimer[0].topic == Event_Null) {
        return EosTimer_Empty;
    }

    // 时间未到达
    if (system_time < eos.timeout_min)
        return EosTimer_NotTimeout;
    // 若时间到达，将此事件推入事件队列，同时在etimer里删除。
    for (uint32_t i = 0; i < eos.timer_count; i ++) {
        if (eos.etimer[i].timeout_ms > system_time)
            continue;
        eos_event_pub_topic(eos.etimer[i].topic);
        // 清零标志位
        if (eos.etimer[i].oneshoot != EOS_False) {
            if (i == (eos.timer_count - 1)) {
                eos.timer_count -= 1;
                break;
            }
            eos.etimer[i] = eos.etimer[eos.timer_count - 1];
            eos.timer_count -= 1;
            i --;
        }
        else {
            uint32_t period = eos.etimer[i].period * timer_unit[eos.etimer[i].unit];
            eos.etimer[i].timeout_ms += period;
        }
    }
    if (eos.timer_count == 0) {
        eos.timeout_min = UINT32_MAX;
        return EosTimer_ChangeToEmpty;
    }

    // 寻找到最小的时间定时器
    uint32_t min_time_out_ms = UINT32_MAX;
    for (uint32_t i = 0; i < eos.timer_count; i ++) {
        if (min_time_out_ms <= eos.etimer[i].timeout_ms)
            continue;
        min_time_out_ms = eos.etimer[i].timeout_ms;
    }
    eos.timeout_min = min_time_out_ms;

    return EosRun_OK;
}
#endif

void eos_delay_ms(uint32_t time_ms)
{
    uint32_t bit;
    eos_critical_enter();

    /* never call eos_delay_ms and eos_delay_ticks in the idle task */
    EOS_ASSERT(eos_current != &task_idle);

    ((eos_task_t *)eos_current)->timeout = eos.time + time_ms;
    bit = (1U << (((eos_task_t *)eos_current)->priority));
    eos.delay |= bit;
    eos_critical_exit();
    
    eos_sheduler();
}

static int8_t eos_get_current(void)
{
    if (eos.init_end == 0) {
        return (int8_t)EosRunErr_NotInitEnd;
    }

    if (eos.enabled == EOS_False) {
#if (EOS_USE_TIME_EVENT != 0)
        eos.timer_count = 0;
#endif
        return (int8_t)EosRun_NotEnabled;
    }

    // 检查是否有状态机的注册
    if (eos.task_exist == 0 || eos.actor_enabled == 0) {
        return (int8_t)EosRun_NoActor;
    }

    if (eos.heap.empty != EOS_False) {
        return (int8_t)EosRun_NoEvent;
    }

    // 寻找到优先级最高，没有挂起，且有事件需要处理的Actor
    uint8_t priority = EOS_MAX_TASKS;
    for (int8_t i = (int8_t)(EOS_MAX_TASKS - 1); i >= 0; i --) {
        if ((eos.task_exist & (1 << i)) == 0)
            continue;
        if ((eos.heap.sub_general & (1 << i)) == 0)
            continue;
        if ((eos.delay & (1 << i)) != 0)
            continue;
        priority = i;
        break;
    }
    // 如果没有找到，返回
    if (priority == EOS_MAX_TASKS) {
        return (int8_t)EosRun_NoActorSub;
    }
    
    return priority + EosRet_Max;
}

int8_t eos_execute(uint8_t priority)
{
    // 寻找当前Actor的最老的事件
    eos_critical_enter();
    eos_event_inner_t * e = eos_heap_get_block(&eos.heap, priority);
    if (e == EOS_NULL) {
        EOS_ASSERT(0);
    }
    eos_critical_exit();
    
    eos_task_t *task = eos.task[priority]->block.task;
    
    // 生成事件
    eos_event_t event;
    event.topic = e->topic;
    event.data = (void *)((eos_pointer_t)e + sizeof(eos_event_inner_t));
    eos_block_t *block = (eos_block_t *)((eos_pointer_t)e - sizeof(eos_block_t));
    event.size = block->size - block->offset - sizeof(eos_event_inner_t);
    // 对事件进行执行
#if (EOS_USE_PUB_SUB != 0)
    uint16_t index = eos_hash_get_index(e->topic);
    if ((eos.hash.object[index].block.event_sub & (1 << priority)) != 0)
#endif
    {
#if (EOS_USE_SM_MODE != 0)
        uint8_t type = eos.task[priority]->type;
        if (type == EosObj_StateMachine) {
            // 执行状态的转换
            eos_sm_t *sm = (eos_sm_t *)task;
            eos_sm_dispath(sm, &event);
        }
        else 
#endif
        if(type == EosObj_Reactor)
        {
            eos_reactor_t *reactor = (eos_reactor_t *)task;
            reactor->event_handler(reactor, &event);
        }
        else {
            EOS_ASSERT(0);
        }
    }
#if (EOS_USE_PUB_SUB != 0)
    else {
        return (int8_t)EosRunErr_ActorNotSub;
    }
#endif
#if (EOS_USE_EVENT_DATA != 0)
    // 销毁过期事件与其携带的参数
    eos_critical_enter();
    eos_heap_gc(&eos.heap, e);
    eos_critical_exit();
#endif

    return (int8_t)EosRun_OK;
}

void eos_task_start(eos_task_t * const me,
                    const char *name,
                    eos_func_t func,
                    uint8_t priority,
                    void *stack_addr,
                    uint32_t stack_size)
{
    eos_critical_enter();
    uint16_t index = eos_task_init(me, name, priority, stack_addr, stack_size);
    eos.hash.object[index].block.task = me;
    eos.hash.object[index].type = EosObj_Task;

    eos_task_start_private(me, func, me->priority, stack_addr, stack_size);
    
    if (eos_current == &task_idle) {
        eos_critical_exit();
        eos_sheduler();
    }
    else {
        eos_critical_exit();
    }
}

static void eos_actor_start(eos_task_t * const me,
                            eos_func_t func,
                            uint8_t priority,
                            void *stack_addr,
                            uint32_t stack_size)
{
    eos_critical_enter();
    eos_task_start_private(me, func, me->priority, stack_addr, stack_size);
    
    if (eos_current == &task_idle) {
        eos_critical_exit();
        eos_sheduler();
    }
    else {
        eos_critical_exit();
    }
}


void eos_task_exit(void)
{
    eos_critical_enter();
    eos.task[eos_current->priority]->key = (const char *)0;
    eos.task[eos_current->priority] = (void *)0;
    eos.task_exist &= ~(1 << eos_current->priority);
    eos_critical_exit();
    
    eos_sheduler();
}

static void eos_task_function(void *parameter)
{
    (void)parameter;
    
    while (1) {
        int8_t ret = eos_get_current();
        EOS_ASSERT(ret >= 0);
        
        if (ret >= EosRet_Max) {
            eos_execute(ret - EosRet_Max);
        }
        eos_sheduler();
    }
}

static void eos_sheduler(void)
{
    eos_critical_enter();
    /* eos_next = ... */
    eos_next = &task_idle;
    for (int8_t i = (EOS_MAX_TASKS - 1); i >= 1; i --) {
        if ((eos.heap.sub_general & (1 << i)) != 0 &&
            (eos.task[i]->type == EosObj_Reactor || eos.task[i]->type == EosObj_StateMachine)) {
            eos_next = eos.task[i]->block.task;
            break;
        }
        if ((eos.task_exist & (1 << i)) != 0 &&
            (eos.delay & (1 << i)) == 0 &&
            eos.task[i]->type == EosObj_Task) {
            eos_next = eos.task[i]->block.task;
            break;
        }
    }

    /* trigger PendSV, if needed */
    if (eos_next != eos_current) {
        eos_port_task_switch();
    }
    eos_critical_exit();
}

void eos_run(void)
{
    eos_hook_start();
    
    eos_sheduler();
}

#if (EOS_USE_TIME_EVENT != 0)
uint32_t eos_time(void)
{
    return eos.time;
}

void eos_tick(void)
{
    uint32_t system_time = eos.time, system_time_bkp = eos.time;
    uint32_t offset = EOS_MS_NUM_30DAY - 1 + EOS_TICK_MS;
    system_time = ((system_time + EOS_TICK_MS) % EOS_MS_NUM_30DAY);
    if (system_time_bkp >= (EOS_MS_NUM_30DAY - EOS_TICK_MS) && system_time < EOS_TICK_MS) {
        eos_critical_enter();
        EOS_ASSERT(eos.timeout_min >= offset);
        eos.timeout_min -= offset;
        for (uint32_t i = 0; i < eos.timer_count; i ++) {
            EOS_ASSERT(eos.etimer[i].timeout_ms >= offset);
            eos.etimer[i].timeout_ms -= offset;
        }
        eos_critical_exit();
    }
    eos.time = system_time;
    
#if (EOS_USE_TIME_EVENT != 0)
    eos_evttimer();
#endif

    /* check all the time-events are timeout or not */
    uint32_t working_set, bit;
    working_set = eos.delay;
    while (working_set != 0U) {
        eos_task_t *t = eos.task[LOG2(working_set) - 1]->block.task;
        EOS_ASSERT(t != (eos_task_t *)0);
        EOS_ASSERT(((eos_task_t *)t)->timeout != 0U);

        bit = (1U << (((eos_task_t *)t)->priority));
        if (eos.time >= ((eos_task_t *)t)->timeout) {
            eos.delay &= ~bit;              /* remove from set */
        }
        working_set &=~ bit;                /* remove from working set */
    }
    
    if (eos_current == &task_idle) {
        eos_sheduler();
    }
}
#endif

// 关于Reactor -----------------------------------------------------------------
static uint16_t eos_task_init(  eos_task_t * const me,
                                const char *name,
                                uint8_t priority,
                                void *stack, uint32_t size)
{
    // 框架需要先启动起来
    EOS_ASSERT(eos.enabled != EOS_False);
    EOS_ASSERT(eos.running == EOS_False);
    // 参数检查
    EOS_ASSERT(me != (eos_task_t *)0);
    EOS_ASSERT(priority < EOS_MAX_TASKS);
    // 防止二次启动
    EOS_ASSERT(me->enabled == EOS_False);
    // 检查优先级的重复注册
    EOS_ASSERT((eos.task_exist & (1 << priority)) == 0);
    // 检查重名
    EOS_ASSERT(eos_hash_get_index(name) == EOS_MAX_OBJECTS);
    // 获取哈希表的位置
    uint16_t index = eos_hash_insert(name);
    eos.hash.object[index].block.task = me;
    // 注册到框架里
    eos.task_exist |= (1 << priority);
    eos.task[priority] = &(eos.hash.object[index]);
    // 状态机
    me->priority = priority;
    me->stack = stack;
    me->size = size;

    return index;
}

void eos_reactor_init(  eos_reactor_t * const me,
                        const char *name,
                        uint8_t priority,
                        void *stack, uint32_t size)
{
    uint16_t index = eos_task_init(&me->super, name, priority, stack, size);
    eos.hash.object[index].type = EosObj_Reactor;
}

void eos_reactor_start(eos_reactor_t * const me, eos_event_handler event_handler)
{
    me->event_handler = event_handler;
    me->super.enabled = EOS_True;
    eos.actor_enabled |= (1 << me->super.priority);
    
    eos_actor_start(&me->super,
                    eos_task_function,
                    me->super.priority,
                    me->super.stack, me->super.size);
}

// state machine ---------------------------------------------------------------
#if (EOS_USE_SM_MODE != 0)
void eos_sm_init(   eos_sm_t * const me,
                    const char *name,
                    uint8_t priority,
                    void *stack, uint32_t size)
{
    uint16_t index = eos_task_init(&me->super, name, priority, stack, size);
    eos.hash.object[index].type = EosObj_StateMachine;
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
    int32_t ip = 0;
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
    
    eos_actor_start(&me->super,
                    eos_task_function,
                    me->super.priority,
                    me->super.stack, me->super.size);
}
#endif

// event -----------------------------------------------------------------------
bool eos_event_topic(eos_event_t const * const e, const char *topic)
{
    return (strcmp(e->topic, topic) == 0) ? true : false;
}

int8_t eos_event_pub_ret(const char *topic, void *data, uint32_t size)
{
    if (eos.init_end == EOS_False) {
        return (int8_t)EosRunErr_NotInitEnd;
    }

    // 保证框架已经运行
    if (eos.enabled == EOS_False) {
        return (int8_t)EosRun_NotEnabled;
    }

    if (size == 0 && data != EOS_NULL) {
        return (int8_t)EosRunErr_InvalidEventData;
    }

    if (eos.task_exist == EOS_False) {
        return (int8_t)EosRun_NoActor;
    }

    // 没有状态机使能，返回
    if (eos.actor_enabled == EOS_False) {
        return (int8_t)EosRun_NotEnabled;
    }

    eos_critical_enter();
    // 申请事件空间
    eos_event_inner_t *e = eos_heap_malloc(&eos.heap, (size + sizeof(eos_event_inner_t)));
    if (e == (eos_event_inner_t *)0) {
        eos_critical_exit();
        return (int8_t)EosRunErr_MallocFail;
    }
    e->topic = topic;
#if (EOS_USE_PUB_SUB != 0)
    uint16_t index = eos_hash_get_index(topic);
    EOS_ASSERT(index != EOS_MAX_OBJECTS);
    e->sub = eos.hash.object[index].block.event_sub;
#else
    e->sub = eos.task_exist;
#endif
    eos.heap.sub_general |= e->sub;
    uint8_t *e_data = (uint8_t *)e + sizeof(eos_event_inner_t);
    for (uint32_t i = 0; i < size; i ++) {
        e_data[i] = ((uint8_t *)data)[i];
    }
    eos_critical_exit();
    
    return (int8_t)EosRun_OK;
}

void eos_event_pub_topic(const char *topic)
{
    int8_t ret = eos_event_pub_ret(topic, EOS_NULL, 0);
    EOS_ASSERT(ret >= 0);
    (void)ret;
    
    if (eos_current == &task_idle) {
        eos_sheduler();
    }
}

#if (EOS_USE_EVENT_DATA != 0)
void eos_event_pub(const char *topic, void *data, uint32_t size)
{
    int8_t ret = eos_event_pub_ret(topic, data, size);
    EOS_ASSERT(ret >= 0);
    (void)ret;
    
    if (eos_current == &task_idle) {
        eos_sheduler();
    }
}
#endif

#if (EOS_USE_PUB_SUB != 0)
void eos_event_sub(eos_task_t * const me, const char *topic)
{
    // 通过Topic找到对应的Object
    uint16_t index;
    index = eos_hash_get_index(topic);
    if (index == EOS_MAX_OBJECTS) {
        index = eos_hash_insert(topic);
        eos.hash.object[index].type = EosObj_Event;
    }
    else {
        EOS_ASSERT(eos.hash.object[index].type == EosObj_Event);
    }

    // 写入订阅
    eos.hash.object[index].block.event_sub |= (1 << me->priority);
}

void eos_event_unsub(eos_task_t * const me, const char *topic)
{
    // 通过Topic找到对应的Object
    uint16_t index = eos_hash_insert(topic);
    EOS_ASSERT(eos.hash.object[index].type == EosObj_Event);

    // 删除订阅
    eos.hash.object[index].block.event_sub &=~ (1 << me->priority);
}
#endif

#if (EOS_USE_TIME_EVENT != 0)
void eos_event_pub_time(const char *topic, uint32_t time_ms, eos_bool_t oneshoot)
{
    EOS_ASSERT(time_ms != 0);
    EOS_ASSERT(time_ms <= timer_threshold[EosTimerUnit_Minute]);
    EOS_ASSERT(eos.timer_count < EOS_MAX_TIME_EVENT);

    // 检查重复，不允许重复发送。
    for (uint32_t i = 0; i < eos.timer_count; i ++) {
        EOS_ASSERT(topic != eos.etimer[i].topic);
    }

    uint32_t system_ms = eos.time;
    uint8_t unit = EosTimerUnit_Ms;
    uint16_t period;
    for (uint8_t i = 0; i < EosTimerUnit_Max; i ++) {
        if (time_ms > timer_threshold[i])
            continue;
        unit = i;
        
        if (i == EosTimerUnit_Ms) {
            period = time_ms;
            break;
        }
        period = (time_ms + (timer_unit[i] >> 1)) / timer_unit[i];
        break;
    }
    uint32_t timeout = (system_ms + time_ms);
    eos.etimer[eos.timer_count ++] = (eos_event_timer_t) {
        topic, oneshoot, unit, period, timeout
    };
    
    if (eos.timeout_min > timeout) {
        eos.timeout_min = timeout;
    }
}

void eos_event_pub_delay(const char *topic, uint32_t time_ms)
{
    eos_event_pub_time(topic, time_ms, EOS_True);
}

void eos_event_pub_period(const char *topic, uint32_t time_ms_period)
{
    eos_event_pub_time(topic, time_ms_period, EOS_False);
}

void eos_event_time_cancel(const char *topic)
{
    uint32_t timeout_min = UINT32_MAX;
    for (uint32_t i = 0; i < eos.timer_count; i ++) {
        if (topic != eos.etimer[i].topic) {
            timeout_min =   timeout_min > eos.etimer[i].timeout_ms ?
                            eos.etimer[i].timeout_ms :
                            timeout_min;
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

    int32_t ip = eos_sm_tran(me, path); // take the HSM transition

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
            HSM_TRIG_(path[ip --], Event_Enter);       // 进入path[ip]
        } while (ip >= 0);

        t = path[0];
    }

    me->state = t;                                  // 更新当前状态
#endif
}

#if (EOS_USE_HSM_MODE != 0)
static int32_t eos_sm_tran(eos_sm_t * const me, eos_state_handler path[EOS_MAX_HSM_NEST_DEPTH])
{
    // transition entry path index
    int32_t ip = -1;
    int32_t iq; // helper transition entry path index
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

/* hash table library ------------------------------------------------------- */


/* heap library ------------------------------------------------------------- */
void eos_heap_init(eos_heap_t * const me)
{
    eos_block_t * block_1st;
    
    // block start
    me->queue = EOS_SIZE_HEAP;
    me->error_id = 0;
    me->size = EOS_SIZE_HEAP;
    me->empty = EOS_True;
    me->sub_general = 0;
    me->current = EOS_SIZE_HEAP;

    memset(me->data, 0, EOS_SIZE_HEAP);

    // the 1st free block
    block_1st = (eos_block_t *)(me->data);
   
    block_1st->last = EOS_SIZE_HEAP;
    block_1st->size = EOS_SIZE_HEAP - (uint16_t)sizeof(eos_block_t);
    block_1st->free = EOS_True;
    block_1st->next = EOS_SIZE_HEAP;
}

void * eos_heap_malloc(eos_heap_t * const me, uint32_t size)
{
    eos_block_t * block;
    int16_t remaining;

    if (size == 0) {
        me->error_id = 1;
        return EOS_NULL;
    }

    /* Find the first free block in the block-list. */
    uint16_t next = 0;
    do {
        block = (eos_block_t *)(me->data + next);
        remaining = (block->size - size - sizeof(eos_block_t));
        if (block->free != EOS_False && remaining >= 0) {
            break;
        }
        next = block->next;
    } while (next != EOS_SIZE_HEAP);

    if (next == EOS_SIZE_HEAP) {
        me->error_id = 2;
        return EOS_NULL;
    }

    /* Divide the block into two blocks. */
    /* ARM Cortex-M0不支持非对齐访问 */
    uint8_t offset = (size % 4);
    size = (offset == 0) ? size : (size + 4 - offset);
    eos_pointer_t address = (eos_pointer_t)block + size + sizeof(eos_block_t);
    eos_block_t * new_block = (eos_block_t *)address;
    uint32_t _size = block->size - size - sizeof(eos_block_t);

    /* Update the list. */
    new_block->size = _size;
    new_block->free = EOS_True;
    new_block->next = block->next;
    new_block->last = (uint16_t)((eos_pointer_t)block - (eos_pointer_t)me->data);

    block->next = (uint16_t)((eos_pointer_t)new_block - (eos_pointer_t)me->data);
    block->size = size;
    block->free = EOS_False;
    block->offset = (offset == 0) ? 0 : (4 - offset);

    if (new_block->next != EOS_SIZE_HEAP) {
        eos_block_t * block_next2 = (eos_block_t *)((eos_pointer_t)me->data + new_block->next);
        block_next2->last = (uint16_t)((eos_pointer_t)new_block - (eos_pointer_t)me->data);
    }

    /* 挂在Queue的最后端 */
    next = me->queue;
    eos_block_t * block_queue;
    if (me->queue == EOS_SIZE_HEAP) {
        me->queue = (uint16_t)((eos_pointer_t)block - (eos_pointer_t)me->data);
        block->q_next = EOS_SIZE_HEAP;
        block->q_last = EOS_SIZE_HEAP;
        me->current = me->queue;
    }
    else {
        do {
            block_queue = (eos_block_t *)(me->data + next);
            next = block_queue->q_next;
        } while (next != EOS_SIZE_HEAP);

        block_queue->q_next = (uint16_t)((eos_pointer_t)block - (eos_pointer_t)me->data);
        block->q_next = EOS_SIZE_HEAP;
        block->q_last = (uint16_t)((eos_pointer_t)block_queue - (eos_pointer_t)me->data);
    }

    me->error_id = 0;
    me->empty = EOS_False;
    void *p = (void *)((eos_pointer_t)block + (uint32_t)sizeof(eos_block_t));
    me->count ++;

    return p;
}

void eos_heap_gc(eos_heap_t * const me, void *data)
{
    eos_event_inner_t *e = (eos_event_inner_t *)data;

    if (e->sub == 0) {
        eos_block_t *block = (eos_block_t *)((eos_pointer_t)data - sizeof(eos_block_t));
        uint16_t index = (uint16_t)((eos_pointer_t)block - (eos_pointer_t)me->data);
        eos_block_t *block_last = (eos_block_t *)(me->data + block->q_last);
        eos_block_t *block_next = (eos_block_t *)(me->data + block->q_next);

        /* 从Queue中删除 */
        // 如果当前只有这一个block
        if (block->q_next == EOS_SIZE_HEAP && block->q_last == EOS_SIZE_HEAP) {
            me->empty = EOS_True;
            me->current = EOS_SIZE_HEAP;
            me->queue = EOS_SIZE_HEAP;
        }
        // 如果这个block在Queue的第一个
        else if (me->queue == index) {
            block_next->q_last = EOS_SIZE_HEAP;
            me->queue = block->q_next;
            me->current = block->q_next;
        }
        // 如果这个block在Queue的最后一个
        else if (block->q_next == EOS_SIZE_HEAP) {
            block_last->q_next = EOS_SIZE_HEAP;
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
    uint16_t next = me->queue;
    uint16_t loop_count = 0;
    eos_block_t *block;
    while (next != EOS_SIZE_HEAP && loop_count < me->count) {
        eos_event_inner_t *evt;
        block = (eos_block_t *)((eos_pointer_t)me->data + next);
        evt = (eos_event_inner_t *)((eos_pointer_t)block + sizeof(eos_block_t));
        me->sub_general |= evt->sub;
        next = block->q_next;

        loop_count ++;
    }
}

void *eos_heap_get_block(eos_heap_t * const me, uint8_t priority)
{
    eos_block_t * block = EOS_NULL;
    eos_event_inner_t *e = EOS_NULL;

    EOS_ASSERT(priority < EOS_MAX_TASKS);

    uint16_t next = me->current;
    uint16_t loop_count = 0;
    while (next != EOS_SIZE_HEAP && loop_count < me->count) {
        eos_event_inner_t *evt;
        block = (eos_block_t *)((eos_pointer_t)me->data + next);
        EOS_ASSERT(block->free == EOS_False);
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
    if (block->last != EOS_SIZE_HEAP) {
        eos_block_t * block_last = (eos_block_t *)(me->data + block->last);
        /* Check the block can be combined with the front one. */
        if (block_last->free != EOS_False) {
            block_last->next = block->next;
            if (block->next != EOS_SIZE_HEAP) {
                block_next = (eos_block_t *)(me->data + block_last->next);
                block_next->last = (uint16_t)((eos_pointer_t)block_last - (eos_pointer_t)me->data);
            }
            block_last->size += (block->size + sizeof(eos_block_t));
            block = block_last;
        }
    }
    
    /* Check the block can be combined with the later one. */
    if (block->next != EOS_SIZE_HEAP) {
        eos_block_t * block_next = (eos_block_t *)(me->data + block->next);
        eos_block_t * block_next2;
        if (block_next->free != EOS_False) {
            block->size += (block_next->size + (uint32_t)sizeof(eos_block_t));
            block->next = block_next->next;
            if (block->next != EOS_SIZE_HEAP) {
                block_next2 = (eos_block_t *)(me->data + block_next->next);
                block_next2->last = (uint16_t)((eos_pointer_t)block - (eos_pointer_t)me->data);
            }
        }
    }

    block->free = EOS_True;
    me->count --;
}

static uint32_t eos_hash_time33(const char *string)
{
    uint32_t hash = 5381;
    while (*string) {
        hash += (hash << 5) + (*string ++);
    }

    return (uint32_t)(hash & INT32_MAX);
}

static uint16_t eos_hash_insert(const char *string)
{
    uint16_t index = 0;

    // 计算哈希值。
    uint32_t hash = eos.hash_func(string);
    uint16_t index_init = hash % eos.prime_max;

    for (uint16_t i = 0; i < (EOS_MAX_OBJECTS / 2 + 1); i ++) {
        for (int8_t j = -1; j <= 1; j += 2) {
            index = index_init + i * j + 2 * (int16_t)EOS_MAX_OBJECTS;
            index %= EOS_MAX_OBJECTS;

            // 寻找到空的哈希
            if (eos.hash.object[index].key == (const char *)0) {
                eos.hash.object[index].key = string;
                return index;
            }
            if (strcmp(eos.hash.object[index].key, string) != 0) {
                continue;
            }
            
            return index;
        }

        // 确保哈希表的查找速度
        if (i >= EOS_MAX_HASH_SEEK_TIMES) {
            // 需要加大哈希表的容量。
            EOS_ASSERT(0);
        }
    }

    // 需要加大哈希表的容量。
    EOS_ASSERT(0);
    
    return 0;
}

static uint16_t eos_hash_get_index(const char *string)
{
    uint16_t index = 0;

    // 计算哈希值。
    uint32_t hash = eos.hash_func(string);
    uint16_t index_init = hash % eos.prime_max;

    for (uint16_t i = 0; i < (EOS_MAX_OBJECTS / 2 + 1); i ++) {
        for (int8_t j = -1; j <= 1; j += 2) {
            index = index_init + i * j + 2 * (int16_t)EOS_MAX_OBJECTS;
            index %= EOS_MAX_OBJECTS;

            if (eos.hash.object[index].key == (const char *)0) {
                continue;
            }
            if (strcmp(eos.hash.object[index].key, string) != 0) {
                continue;
            }
            
            return index;
        }

        // 确保哈希表的查找速度
        if (i >= EOS_MAX_HASH_SEEK_TIMES) {
            return EOS_MAX_OBJECTS;
        }
    }
    
    return EOS_MAX_OBJECTS;
}

/* for unittest ------------------------------------------------------------- */
void * eos_get_framework(void)
{
    return (void *)&eos;
}

#if (EOS_USE_TIME_EVENT != 0)
void eos_set_time(uint32_t time_ms)
{
    eos.time = time_ms;
}
#endif

#ifdef __cplusplus
}
#endif
