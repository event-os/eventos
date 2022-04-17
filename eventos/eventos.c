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
 * 2021-03-20     DogMing       V0.2.0
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
enum {
    EosObj_Task = 0,
    EosObj_Event,
    EosObj_Timer,
    EosObj_Device,
    EosObj_Heap,
    EosObj_Other,
};

enum {
    EosEventGiveType_Send = 0,
    EosEventGiveType_Publish,
    EosEventGiveType_Broadcast,
};

enum {
    EosTaskState_Ready = 0,
    EosTaskState_Suspend,
    EosTaskState_Running,
    EosTaskState_Delay,
    EosTaskState_DelayNoEvent,
    EosTaskState_WaitEvent,
    EosTaskState_WaitSpecificEvent,
    
    EosTaskState_Max
};

#define EOS_MAGIC                       0xDEADBEEFU

/* eos task ----------------------------------------------------------------- */
eos_task_t *volatile eos_current;
eos_task_t *volatile eos_next;

// **eos** ---------------------------------------------------------------------
// TODO 优化。重新考虑以下枚举是否还有价值。
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

// Event atrribute -------------------------------------------------------------
#define EOS_EVENT_ATTRIBUTE_GLOBAL          ((uint8_t)0x80U)
#define EOS_EVENT_ATTRIBUTE_UNBLOCKED       ((uint8_t)0x40U)
#define EOS_EVENT_ATTRIBUTE_TOPIC           ((uint8_t)0x00U)
#define EOS_EVENT_ATTRIBUTE_VALUE           ((uint8_t)0x01U)
#define EOS_EVENT_ATTRIBUTE_STREAM          ((uint8_t)0x02U)

// Task attribute --------------------------------------------------------------
#define EOS_TASK_ATTRIBUTE_TASK             ((uint8_t)0x00U)
#define EOS_TASK_ATTRIBUTE_REACTOR          ((uint8_t)0x01U)
#define EOS_TASK_ATTRIBUTE_SM               ((uint8_t)0x02U)

typedef uint32_t (* hash_algorithm_t)(const char *string);

#if (EOS_USE_TIME_EVENT != 0)
#define EOS_MS_NUM_30DAY                    (2592000000U)
#define EOS_MS_NUM_15DAY                    (1296000000U)

// TODO 优化。定时器考虑使用uint64_t，并以微秒为单位。以增强时间的精确度。
// TODO 优化。内存充分的前提下，不再搞这个东西。
enum {
    EosTimerUnit_Ms                         = 0,    // 60S, ms
    EosTimerUnit_100Ms,                             // 100Min, 50ms
    EosTimerUnit_Sec,                               // 16h, 500ms
    EosTimerUnit_Minute,                            // 15day, 30S

    EosTimerUnit_Max
};

// TODO 优化。内存充分的前提下，不再搞这个东西。
static const uint32_t timer_threshold[EosTimerUnit_Max] = {
    60000,                                          // 60 S
    6000000,                                        // 100 Minutes
    57600000,                                       // 16 hours
    1296000000,                                     // 15 days
};

// TODO 优化。内存充分的前提下，不再搞这个东西。
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

// TODO。优化。删除这个数据结构。
typedef struct eos_heap_block {
    struct eos_heap_block *next;
    uint32_t is_free                            : 1;
    uint32_t size;
} eos_heap_block_t;

// TODO。优化。删除这个数据结构。
typedef struct eos_heap_tag {
    uint8_t data[EOS_SIZE_HEAP];
    eos_heap_block_t * list;
    uint32_t size;                              /* total size */
    uint32_t error_id                           : 3;
} eos_heap_t;

// TODO 优化。删除next和last指针。同时，heap库也不在有必要。
typedef struct eos_event_data {
    struct eos_event_data *next;
    struct eos_event_data *last;
    uint32_t owner;
    uint16_t id;
    uint8_t type;
} eos_event_data_t;

enum {
    Stream_OK                       = 0,
    Stream_Empty                    = -1,
    Stream_Full                     = -2,
    Stream_NotEnough                = -3,
    Stream_MemCovered               = -4,
};

typedef struct eos_stream {
    void *data;
    uint32_t head;
    uint32_t tail;
    bool empty;

    uint32_t capacity;
} eos_stream_t;

typedef union eos_obj_block {
    // TODO 优化。Event对应的OCB优化为以下样子。
    struct {
        uint32_t sub;
        // TODO 优化。如果支持无限任务。订阅标志位按照以下来。
        uint8_t sub_flag[EOS_MAX_TASKS / 8];
        uint16_t e_id;
        void *data;
        uint32_t attribute              : 8;
        uint32_t size                   : 16;               // Value size
    } e;
    // TODO 优化。Task对应的OCB优化为以下样子。
    struct {
        eos_task_t *task;
        const char *e_wait;
        uint32_t attribute              : 8;
        // TODO 优化。用于支持无限任务。
        uint32_t id;
    } t;
    // TODO 优化。Timer对应的OCB优化为以下样子。
    struct {
        uint32_t time;
        uint32_t time_out;
        eos_func_t callback;
        uint32_t timer_id               : 16;
        uint32_t oneshoot               : 1;
        uint32_t running                : 1;
    } tim;
    struct {
        eos_event_data_t *list;
        uint32_t sub;
    } event;
    eos_task_t *task;
    eos_timer_t *timer;
} eos_ocb_t;

typedef struct eos_object {
    const char *key;                                    // Key
    eos_ocb_t ocb;                                      // object block
    uint32_t type                   : 8;                // Object type
    // TODO 优化。attribute优化进入ocb
    uint32_t attribute              : 8;
    // TODO 优化。放进ocb。
    uint32_t size                   : 16;               // Value size
    // TODO 优化。放进OCB。
    union {
        void *value;                                    // for value-event
        eos_stream_t *stream;                           // for stream-event
    } data;
} eos_object_t;

typedef struct eos_hash_table {
    // TODO 优化。将这些成员直接放进eos中。
    eos_object_t object[EOS_MAX_OBJECTS];
    uint32_t prime_max;                                 // prime max
    uint32_t size;
} eos_hash_table_t;

typedef struct eos_tag {
    // Hash table
    eos_hash_table_t hash;
    hash_algorithm_t hash_func;
    uint16_t prime_max;

    // Task
    eos_object_t *task[EOS_MAX_TASKS];
    // TODO 优化。将此处优化到OCB里去。
    const char *event_wait[EOS_MAX_TASKS];                 // 等待的事件ID
    // TODO 优化。如果支持无限任务，就支持如下格式的标志位。同时，任务数必须为8的倍数。
    uint8_t flag[EOS_MAX_TASKS / 8];
    uint32_t task_exist;
    uint32_t task_enabled;
    uint32_t task_delay;
    uint32_t task_suspend;
    uint32_t task_delay_no_event;
    uint32_t task_wait_event;
    uint32_t task_wait_specific_event;
    
    // Timer
    eos_timer_t *timers;
    uint32_t timer_out_min;

    // Time event
    // TODO 优化。将此处优化到哈希表里去，最好是跟Timer合为一体。
#if (EOS_USE_TIME_EVENT != 0)
    eos_event_timer_t etimer[EOS_MAX_TIME_EVENT];
    uint32_t time;
    uint32_t timeout_min;
    uint64_t time_offset;
    uint8_t timer_count;
#endif

    // Heap
    // TODO 优化。删除heap。
#if (EOS_USE_EVENT_DATA != 0)
    eos_heap_t heap;
#endif
    // TODO 优化。e-queue改为全静态管理。
    eos_event_data_t *e_queue;

    uint32_t owner_global;

    // TODO 优化。用于支持无限任务。
    uint32_t id_count;

    // flag
    uint8_t enabled                        : 1;
    uint8_t running                        : 1;
    uint8_t init_end                       : 1;
} eos_t;

/* eventos API for test ----------------------------- */
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
    {"Event_Null", 0, 0},
    {"Event_Enter", 0, 0},
    {"Event_Exit", 0, 0},
#if (EOS_USE_HSM_MODE != 0)
    {"Event_Init", 0, 0},
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
static void __eos_e_queue_delete(eos_event_data_t const *item);
static uint16_t eos_task_init(  eos_task_t * const me,
                                const char *name,
                                uint8_t priority,
                                void *stack, uint32_t size);
static void eos_reactor_enter(eos_reactor_t *const me);
static inline void __eos_event_sub(eos_task_t *const me, const char *topic);
static void eos_sm_enter(eos_sm_t *const me);
static void eos_sheduler(void);
static int32_t eos_evttimer(void);
static uint32_t eos_hash_time33(const char *string);
static uint16_t eos_hash_insert(const char *string);
static uint16_t eos_hash_get_index(const char *string);
static bool eos_hash_existed(const char *string);
#if (EOS_USE_SM_MODE != 0)
static void eos_sm_dispath(eos_sm_t * const me, eos_event_t const * const e);
#if (EOS_USE_HSM_MODE != 0)
static int32_t eos_sm_tran(eos_sm_t * const me, eos_state_handler path[EOS_MAX_HSM_NEST_DEPTH]);
#endif
#endif
#if (EOS_USE_EVENT_DATA != 0)
static void eos_heap_init(eos_heap_t * const me);
static void * eos_heap_malloc(eos_heap_t * const me, uint32_t size);
static void eos_heap_free(eos_heap_t * const me, void * data);
#endif

static int32_t eos_stream_init(eos_stream_t *const me, void *memory, uint32_t capacity);
static int32_t eos_stream_push(eos_stream_t * me, void * data, uint32_t size);
static int32_t eos_stream_pull_pop(eos_stream_t * me, void * data, uint32_t size);
static bool eos_stream_full(eos_stream_t * me);
static int32_t eos_stream_size(eos_stream_t * me);
static int32_t eos_stream_empty_size(eos_stream_t * me);

/* -----------------------------------------------------------------------------
EventOS
----------------------------------------------------------------------------- */
static uint64_t stack_idle[32];
static eos_task_t task_idle;

static void task_func_idle(void *parameter)
{
    (void)parameter;

    while (1) {
        eos_critical_enter();
        /* check all the tasks are timeout or not */
        uint32_t working_set, bit;
        working_set = eos.task_delay;
        while (working_set != 0U) {
            eos_task_t *t = eos.task[LOG2(working_set) - 1]->ocb.task;
            EOS_ASSERT(t != (eos_task_t *)0);
            EOS_ASSERT(t->timeout != 0U);

            bit = (1U << (t->priority));
            if (eos.time >= t->timeout) {
                t->state = EosTaskState_Ready;
                eos.task_delay &= ~bit;              /* remove from set */
                eos.task_delay_no_event &= ~bit;
                t->state = EosTaskState_Ready;
            }
            working_set &=~ bit;                /* remove from working set */
        }
        eos_critical_exit();
        eos_sheduler();

        eos_critical_enter();
#if (EOS_USE_TIME_EVENT != 0)
        eos_evttimer();
#endif
        if (eos.time >= EOS_MS_NUM_15DAY) {
            // Adjust all task daley timing.
            for (uint32_t i = 1; i < EOS_MAX_TASKS; i ++) {
                if (eos.task[i] != (void *)0 && ((eos.task_delay & (1 << i)) != 0)) {
                    eos.task[i]->ocb.task->timeout -= eos.time;
                }
            }
            // Adjust all timer's timing.
            eos_timer_t *list = eos.timers;
            while (list != (eos_timer_t *)0) {
                if (list->running != 0) {
                    list->time_out -= eos.time;
                }
                list = list->next;
            }
            eos.timer_out_min -= eos.time;
            // Adjust all event timer's
            eos.timeout_min -= eos.time;
            for (uint32_t i = 0; i < eos.timer_count; i ++) {
                EOS_ASSERT(eos.etimer[i].timeout_ms >= eos.time);
                eos.etimer[i].timeout_ms -= eos.time;
            }
            eos.time_offset += eos.time;
            eos.time = 0;
        }
        eos_critical_exit();

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
    eos.task_enabled = 0;
    eos.hash_func = eos_hash_time33;
    eos.e_queue = EOS_NULL;

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

void eos_run(void)
{
    eos_hook_start();
    
    eos_sheduler();
}

uint64_t eos_time(void)
{
    return (uint64_t)(eos.time + eos.time_offset);
}

void eos_tick(void)
{
    uint32_t time = eos.time;
    time += EOS_TICK_MS;
    eos.time = time;
}

// 仅为单元测试
void eos_set_hash(hash_algorithm_t hash)
{
    eos.hash_func = hash;
}

/* -----------------------------------------------------------------------------
Task
----------------------------------------------------------------------------- */
static void eos_sheduler(void)
{
    eos_critical_enter();
    /* eos_next = ... */
    task_idle.state = EosTaskState_Ready;
    eos_next = &task_idle;
    for (int8_t i = (EOS_MAX_TASKS - 1); i >= 1; i --) {
        // Actor有事件且不被延时
        if ((eos.owner_global & (1 << i)) != 0 &&
            (eos.task_delay & (1 << i)) == 0 &&
            (eos.task_suspend & (1 << i)) == 0 &&
            eos.task[i]->type == EosObj_Task &&
            eos.task[i]->attribute != EOS_TASK_ATTRIBUTE_TASK) {
            eos_next = eos.task[i]->ocb.task;
            break;
        }
        if ((eos.task_exist & (1 << i)) != 0 &&
            (eos.task_delay & (1 << i)) == 0 &&
            (eos.task_suspend & (1 << i)) == 0 &&
            (eos.task_wait_event & (1 << i)) == 0 &&
            (eos.task_wait_specific_event & (1 << i)) == 0 &&
            eos.task[i]->type == EosObj_Task) {
            eos_next = eos.task[i]->ocb.task;
            break;
        }
    }

    /* trigger PendSV, if needed */
    if (eos_next != eos_current) {
        eos_next->state = EosTaskState_Running;
        eos_port_task_switch();
    }
    eos_critical_exit();
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
    eos.hash.object[index].ocb.task = me;
    eos.hash.object[index].type = EosObj_Task;

    eos_task_start_private(me, func, me->priority, stack_addr, stack_size);
    me->state = EosTaskState_Ready;
    
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
    me->state = EosTaskState_Ready;
    
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

static inline void eos_delay_ms_private(uint32_t time_ms, bool no_event)
{
    /* never call eos_delay_ms and eos_delay_ticks in the idle task */
    EOS_ASSERT(eos_current != &task_idle);
    /* The state of current task must be running. */
    EOS_ASSERT(eos_current->state == EosTaskState_Running);

    uint32_t bit;
    eos_critical_enter();
    ((eos_task_t *)eos_current)->timeout = eos.time + time_ms;
    eos_current->state = no_event ? EosTaskState_DelayNoEvent : EosTaskState_Delay;
    bit = (1U << (((eos_task_t *)eos_current)->priority));
    eos.task_delay |= bit;
    if (no_event) {
        eos.task_delay_no_event |= bit;
    }
    eos_critical_exit();
    
    eos_sheduler();
}

void eos_delay_ms(uint32_t time_ms)
{
    eos_delay_ms_private(time_ms, false);
}

void eos_delay_no_event(uint32_t time_ms)
{
    eos_delay_ms_private(time_ms, true);
}

void eos_task_suspend(const char *task)
{
    uint16_t index = eos_hash_get_index(task);
    EOS_ASSERT(index != EOS_MAX_OBJECTS);

    eos_object_t *obj = &eos.hash.object[index];
    EOS_ASSERT(obj->type == EosObj_Task);

    obj->ocb.task->state = EosTaskState_Suspend;
    eos.task_suspend |= (1 << obj->ocb.task->priority);

    eos_sheduler();
}

void eos_task_resume(const char *task)
{
    uint16_t index = eos_hash_get_index(task);
    EOS_ASSERT(index != EOS_MAX_OBJECTS);

    eos_object_t *obj = &eos.hash.object[index];
    EOS_ASSERT(obj->type == EosObj_Task);

    obj->ocb.task->state = EosTaskState_Ready;
    eos.task_suspend &=~ (1 << obj->ocb.task->priority);

    eos_sheduler();
}

bool eos_task_wait_event(eos_event_t * const e_out, uint32_t time_ms)
{
    do {
        eos_critical_enter();
        uint8_t priority = eos_current->priority;
        
        if ((eos.owner_global & (1 << priority)) != 0) {
            // Find the first event data in e-queue and set it as handled.
            eos_event_data_t *e_item = eos.e_queue;
            uint32_t bits = (1 << priority);
            while (e_item != EOS_NULL) {
                if ((e_item->owner & bits) == 0) {
                    continue;
                }

                // Meet one event
                eos_object_t *e_object = &eos.hash.object[e_item->id];
                EOS_ASSERT(e_object->type == EosObj_Event);
                uint8_t type = e_object->attribute & 0x03;
                // Event out
                e_out->topic = e_object->key;
                e_out->eid = ~((uint32_t)e_item);
                if (type == EOS_EVENT_ATTRIBUTE_TOPIC) {
                    e_out->size = 0;
                }
                else if (type == EOS_EVENT_ATTRIBUTE_VALUE) {
                    e_out->size = e_object->size;
                }
                else if (type == EOS_EVENT_ATTRIBUTE_STREAM) {
                    e_out->size = eos_stream_size(e_object->data.stream);
                }

                eos_critical_exit();
                return true;
            }
        }
        else {
            uint32_t bit;
            ((eos_task_t *)eos_current)->timeout = eos.time + time_ms;
            eos_current->state = EosTaskState_WaitEvent;
            bit = (1U << priority);
            eos.task_wait_event |= bit;
            eos_critical_exit();

            eos_sheduler();
        }
    } while (eos.time < eos_current->timeout || time_ms == 0);

    return false;
}

void eos_task_yield(void)
{
    eos_sheduler();
}

void eos_task_delete(const char *task)
{
    eos_critical_enter();
    uint16_t e_id = eos_hash_get_index(task);
    // Ensure the topic is existed in hash table.
    EOS_ASSERT(e_id != EOS_MAX_OBJECTS);
    // Ensure the object is task-type.
    EOS_ASSERT(eos.hash.object[e_id].type == EosObj_Task);
    
    uint8_t priority = eos.hash.object[e_id].ocb.task->priority;
    uint32_t bits = (1 << priority);

    // Delete the task from hash table and task table.
    eos.hash.object[e_id].key = EOS_NULL;
    eos.task[priority] = EOS_NULL;
    // Clear the all flags.
    eos.task_exist &=~ bits;
    eos.task_enabled &=~ bits;
    eos.task_delay &=~ bits;
    eos.task_suspend &=~ bits;
    eos.task_delay_no_event &=~ bits;
    eos.task_wait_event &=~ bits;
    eos.task_wait_specific_event &=~ bits;

    eos_critical_exit();
    
    eos_sheduler();
}

bool eos_task_wait_specific_event(  eos_event_t * const e_out,
                                    const char *topic, uint32_t time_ms)
{
    uint8_t priority = eos_current->priority;
    uint32_t bits = (1 << priority);
    
    do {
        eos_critical_enter();
        uint16_t e_id = eos_hash_get_index(topic);
        // If the topic is not existed in hash table.
        if (e_id == EOS_MAX_OBJECTS) {
            e_id = eos_hash_insert(topic);
            eos.hash.object[e_id].type = EosObj_Event;
            eos.hash.object[e_id].attribute = EOS_EVENT_ATTRIBUTE_TOPIC;
        }
        // If the topic is existed in hash table.
        else {
            /*  Read all items in e_queu and handle all other events as handled
             *  util the specific event comes out. Finally glocal owner flag will
             *  be updated.
             */ 
            eos_event_data_t *e_item = eos.e_queue;
            eos.owner_global = 0;
            while (e_item != EOS_NULL) {
                if ((e_item->owner & bits) == 0) {
                    eos.owner_global |= e_item->owner;
                    continue;
                }
                
                // Meet the specific event.
                if (strcmp(eos.hash.object[e_item->id].key, topic) == 0) {
                    eos_object_t *e_object = &eos.hash.object[e_item->id];
                    EOS_ASSERT(e_object->type == EosObj_Event);
                    uint8_t type = e_object->attribute & 0x03;

                    e_out->topic = topic;
                    e_out->eid = ~((uint32_t)e_item);
                    if (type == EOS_EVENT_ATTRIBUTE_TOPIC) {
                        e_out->size = 0;
                    }
                    else if (type == EOS_EVENT_ATTRIBUTE_VALUE) {
                        e_out->size = e_object->size;
                    }
                    else if (type == EOS_EVENT_ATTRIBUTE_STREAM) {
                        e_out->size = eos_stream_size(e_object->data.stream);
                    }

                    eos_critical_exit();
                    return true;
                }
            }
        }

        eos_current->timeout = eos.time + time_ms;
        eos_current->state = EosTaskState_WaitSpecificEvent;
        eos.task_wait_specific_event |= (1U << priority);
        eos.event_wait[priority] = topic;
        
        eos_critical_exit();

        eos_sheduler();
    } while (eos.time < eos_current->timeout || time_ms == 0);

    return false;
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

    eos_timer_t *current = eos.hash.object[index].ocb.timer;
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
    eos_timer_t *timer = eos.hash.object[index].ocb.timer;
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
    eos_timer_t *timer = eos.hash.object[index].ocb.timer;
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
    eos_timer_t *timer = eos.hash.object[index].ocb.timer;
    timer->running = 1;
    timer->time_out = eos.time + timer->time;
    eos_critical_exit();
}

/* -----------------------------------------------------------------------------
Event
----------------------------------------------------------------------------- */
void eos_event_attribute_global(const char *topic)
{
    eos_critical_enter();
    uint16_t e_id;
    if (eos_hash_existed(topic) == false) {
        e_id = eos_hash_insert(topic);
        eos.hash.object[e_id].type = EosObj_Event;
    }
    EOS_ASSERT(eos.hash.object[e_id].type == EosObj_Event);
    eos.hash.object[e_id].attribute |= EOS_EVENT_ATTRIBUTE_GLOBAL;
    
    eos_critical_exit();
}

void eos_event_attribute_unblocked(const char *topic)
{
    eos_critical_enter();
    uint16_t e_id;
    if (eos_hash_existed(topic) == false) {
        e_id = eos_hash_insert(topic);
        eos.hash.object[e_id].type = EosObj_Event;
    }
    EOS_ASSERT(eos.hash.object[e_id].type == EosObj_Event);
    eos.hash.object[e_id].attribute |= EOS_EVENT_ATTRIBUTE_UNBLOCKED;

    eos_critical_exit();
}

void eos_event_attribute_stream(const char *topic,
                                const char *target,
                                void *memory, uint32_t capacity)
{
    eos_critical_enter();
    uint16_t e_id;
    if (eos_hash_existed(topic) == false) {
        e_id = eos_hash_insert(topic);
        eos.hash.object[e_id].type = EosObj_Event;
    }
    EOS_ASSERT(eos.hash.object[e_id].type == EosObj_Event);
    eos.hash.object[e_id].attribute &=~ 0x03;
    eos.hash.object[e_id].attribute |= EOS_EVENT_ATTRIBUTE_STREAM;
    eos.hash.object[e_id].data.stream = (eos_stream_t *)memory;
    eos.hash.object[e_id].size = capacity - sizeof(eos_stream_t);

    eos_stream_init(eos.hash.object[e_id].data.stream,
                    (void *)((uint32_t)memory + sizeof(eos_stream_t)),
                    eos.hash.object[e_id].size);

    uint16_t t_id = eos_hash_get_index(topic);
    EOS_ASSERT(t_id != EOS_MAX_OBJECTS);
    uint8_t priority = eos.hash.object[e_id].ocb.task->priority;
    eos.hash.object[e_id].ocb.event.sub = (1 << priority);

    eos_critical_exit();
}

void eos_event_attribute_value(const char *topic, void *memory, uint32_t size)
{
    eos_critical_enter();
    uint16_t e_id;
    if (eos_hash_existed(topic) == false) {
        e_id = eos_hash_insert(topic);
        eos.hash.object[e_id].type = EosObj_Event;
    }
    EOS_ASSERT(eos.hash.object[e_id].type == EosObj_Event);
    eos.hash.object[e_id].attribute &=~ 0x03;
    eos.hash.object[e_id].attribute |= EOS_EVENT_ATTRIBUTE_VALUE;
    eos.hash.object[e_id].data.value = memory;
    eos.hash.object[e_id].size = size;

    eos_critical_exit();
}

static inline void eos_event_broadcast_private( const char *topic,
                                                void const *data,
                                                uint8_t type)
{
    eos_critical_enter();

    uint32_t owner = eos.task_exist;

    // Get event id according the topic.
    uint16_t e_id = eos_hash_get_index(topic);
    uint8_t e_type;
    if (e_id == EOS_MAX_OBJECTS) {
        e_id = eos_hash_insert(topic);
        eos.hash.object[e_id].type = EosObj_Event;
        e_type = type;
    }
    // Get the type of the event
    else {
        e_type = eos.hash.object[e_id].type;
        EOS_ASSERT(eos.hash.object[e_id].type == EosObj_Event);
        EOS_ASSERT(e_type == type);
    }

    // 查看是否相关线程，在等待特定事件。
    uint32_t wait_event = owner & eos.task_wait_event;
    uint32_t wait_specific_event = owner & eos.task_wait_specific_event;
    for (uint8_t i = 1; i < EOS_MAX_TASKS; i ++) {
        if ((wait_specific_event & (1 << i)) != 0 &&
            strcmp(topic, eos.event_wait[i]) == 0) {
            eos.task[i]->ocb.task->state = EosTaskState_Ready;
            eos.task_delay &=~ (1 << i);
            eos.task_wait_event &=~ (1 << i);
            eos.task_wait_specific_event &=~ (1 << i);
        }
        else if ((wait_event & (1 << i)) != 0) {
            eos.task[i]->ocb.task->state = EosTaskState_Ready;
            eos.task_delay &=~ (1 << i);
            eos.task_wait_event &=~ (1 << i);
            eos.task_wait_specific_event &=~ (1 << i);
        }
    }
    eos.owner_global |= owner;

    if (e_type == EOS_EVENT_ATTRIBUTE_TOPIC) {
        // Apply one data for the event.
        eos_event_data_t *data = eos_heap_malloc(&eos.heap, sizeof(eos_event_data_t));
        data->id = e_id;
        data->owner = owner;
        EOS_ASSERT(data != EOS_NULL);
        // 挂到事件队列的后面。
        if (eos.e_queue == EOS_NULL) {
            eos.e_queue = data;
            data->next = EOS_NULL;
            data->last = EOS_NULL;
        }
        else {
            eos_event_data_t *edata = eos.e_queue;
            while (edata->next != EOS_NULL) {
                edata = edata->next;
            }
            data->next = EOS_NULL;
            edata->next = data;
            data->last = edata;
        }
    }
    else if (e_type == EOS_EVENT_ATTRIBUTE_VALUE) {
        uint32_t size = eos.hash.object[e_id].size;
        eos_event_data_t *data;
        // 挂到Hash表里去
        eos_event_data_t *list = eos.hash.object[e_id].ocb.event.list;
        if (list == EOS_NULL) {
            // Apply one data for the event.
            data = eos_heap_malloc(&eos.heap, sizeof(eos_event_data_t));
            eos.hash.object[e_id].ocb.event.list = data;
            list = data;
            list->owner = owner;
            
            // 挂到事件队列的后面。
            if (eos.e_queue == EOS_NULL) {
                eos.e_queue = data;
                data->next = EOS_NULL;
                data->last = EOS_NULL;
            }
            else {
                eos_event_data_t *edata = eos.e_queue;
                while (edata->next != EOS_NULL) {
                    edata = edata->next;
                }
                data->next = EOS_NULL;
                edata->next = data;
                data->last = edata;
            }
        }
        else {
            list->owner |= owner;
        }
        // Set the event's value.
        for (uint32_t i = 0; i < size; i ++) {
            ((uint8_t *)(eos.hash.object[e_id].data.value))[i] = ((uint8_t *)data)[i];
        }
    }
    eos_critical_exit();
}

void eos_event_broadcast_topic(const char *topic)
{
    eos_event_broadcast_private(topic, EOS_NULL, EOS_EVENT_ATTRIBUTE_TOPIC);
}

void eos_event_broadcast_value(const char *topic, void const *data)
{
    eos_event_broadcast_private(topic, data, EOS_EVENT_ATTRIBUTE_VALUE);
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

static void eos_task_function(void *parameter)
{
    (void)parameter;

    uint8_t type = eos.task[eos_current->priority]->attribute;
    // Reactor exutes the event Enter.
    if (type == EOS_TASK_ATTRIBUTE_REACTOR) {
        eos_reactor_enter((eos_reactor_t *)eos_current);
    }
    // State machine enter all initial states.
    else if (type == EOS_TASK_ATTRIBUTE_SM) {
        eos_sm_enter((eos_sm_t *)eos_current);
    }
    else {
        EOS_ASSERT(0);
    }

    while (1) {
        eos_event_t e;
        if (eos_task_wait_event(&e, 0)) {
            if (type == EOS_TASK_ATTRIBUTE_SM) {
                eos_sm_dispath((eos_sm_t *)eos_current, &e);
            }
            else if (type == EOS_TASK_ATTRIBUTE_REACTOR) {
                eos_reactor_t *reactor = (eos_reactor_t *)eos_current;
                reactor->event_handler(reactor, &e);
            }
        }
    }
}

static void __eos_e_queue_delete(eos_event_data_t const *item)
{
    EOS_ASSERT(eos.e_queue != EOS_NULL);
    EOS_ASSERT(item != EOS_NULL);
    
    // If the event data is only one in queue.
    if (item->last == EOS_NULL && item->next == EOS_NULL) {
        eos.e_queue = EOS_NULL;
    }
    // If the event data is the first one in queue.
    else if (item->last == EOS_NULL && item->next != EOS_NULL) {
        item->next->last = item->last;
        eos.e_queue = item->next;
    }
    // If the event item is the last one in queue.
    else if (item->last != EOS_NULL && item->next == EOS_NULL) {
        item->last->next = item->next;
    }
    // If the event item is in the middle position of the queue.
    else {
        item->last->next = item->next;
        item->next->last = item->last;
    }
    
    // Calculate the owner_global.
    eos.owner_global = 0;
    eos_event_data_t *e_item = eos.e_queue;
    while (e_item != EOS_NULL) {
        eos.owner_global |= e_item->owner;
        e_item = e_item->next;
    }
}

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
    eos.hash.object[index].ocb.task = me;
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
    eos.hash.object[index].type = EosObj_Task;
    eos.hash.object[index].attribute = EOS_TASK_ATTRIBUTE_REACTOR;
}

void eos_reactor_start(eos_reactor_t * const me, eos_event_handler event_handler)
{
    me->event_handler = event_handler;
    me->super.enabled = EOS_True;
    eos.task_enabled |= (1 << me->super.priority);
    
    eos_event_send_topic(eos.task[me->super.priority]->key, "Event_Null");
    
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
    eos.hash.object[index].type = EosObj_Task;
    eos.hash.object[index].attribute = EOS_TASK_ATTRIBUTE_SM;
    me->state = eos_state_top;
}

void eos_sm_start(eos_sm_t * const me, eos_state_handler state_init)
{
    me->state = state_init;
    me->super.enabled = EOS_True;
    eos.task_enabled |= (1 << me->super.priority);
    
    eos_event_send_topic(eos.task[me->super.priority]->key, "Event_Null");

    eos_actor_start(&me->super,
                    eos_task_function,
                    me->super.priority,
                    me->super.stack, me->super.size);
}
#endif

static void eos_reactor_enter(eos_reactor_t *const me)
{
    eos_event_t e = {
        "Event_Enter", 0, 0,
    };
    me->event_handler(me, &e);
}

static void eos_sm_enter(eos_sm_t *const me)
{
#if (EOS_USE_HSM_MODE != 0)
    eos_state_handler path[EOS_MAX_HSM_NEST_DEPTH];
#endif
    eos_state_handler t;

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
}

/* -----------------------------------------------------------------------------
Event
----------------------------------------------------------------------------- */

static int8_t __eos_event_give(  const char *task,
                                        uint8_t give_type,
                                        const char *topic, 
                                        const void *memory, uint32_t size)
{
    eos_critical_enter();

    // Get event id according the topic.
    uint16_t e_id = eos_hash_get_index(topic);
    uint8_t e_type;
    if (e_id == EOS_MAX_OBJECTS) {
        e_id = eos_hash_insert(topic);
        eos.hash.object[e_id].type = EosObj_Event;
        e_type = EOS_EVENT_ATTRIBUTE_TOPIC;
    }
    else {
        // Get the type of the event
        e_type = eos.hash.object[e_id].attribute & 0x03;
        EOS_ASSERT(eos.hash.object[e_id].type == EosObj_Event);
    }
    
    uint32_t owner = 0;
    if (give_type == EosEventGiveType_Send) {
        if (task != EOS_NULL) {
            uint8_t priority;
            uint16_t t_index = eos_hash_get_index(task);
            EOS_ASSERT(t_index != EOS_MAX_OBJECTS);
            EOS_ASSERT(eos.hash.object[t_index].type == EosObj_Task);
            eos_task_t *tcb = eos.hash.object[t_index].ocb.task;
            priority = tcb->priority;
            owner = (1 << priority);
        }
    }
    else if (give_type == EosEventGiveType_Broadcast) {
        owner = eos.task_exist;
    }
    else if (give_type == EosEventGiveType_Publish) {
        owner = eos.hash.object[e_id].ocb.event.sub;
    }
    else {
        EOS_ASSERT(0);
    }
    eos.owner_global |= owner;

    // 查看是否相关线程，在等待特定事件。
    uint32_t wait_event = owner & eos.task_wait_event;
    uint32_t wait_specific_event = owner & eos.task_wait_specific_event;
    for (uint8_t i = 1; i < EOS_MAX_TASKS; i ++) {
        if ((wait_specific_event & (1 << i)) != 0 &&
            strcmp(topic, eos.event_wait[i]) == 0) {
            eos.task[i]->ocb.task->state = EosTaskState_Ready;
            eos.task_delay &=~ (1 << i);
            eos.task_wait_event &=~ (1 << i);
            eos.task_wait_specific_event &=~ (1 << i);
        }
        else if ((wait_event & (1 << i)) != 0) {
            eos.task[i]->ocb.task->state = EosTaskState_Ready;
            eos.task_delay &=~ (1 << i);
            eos.task_wait_event &=~ (1 << i);
            eos.task_wait_specific_event &=~ (1 << i);
        }
    }

    if (e_type == EOS_EVENT_ATTRIBUTE_TOPIC) {
        // Apply one data for the event.
        eos_event_data_t *data = eos_heap_malloc(&eos.heap, sizeof(eos_event_data_t));
        data->id = e_id;
        data->owner = owner;
        EOS_ASSERT(data != EOS_NULL);
        // 挂到事件队列的后面。
        if (eos.e_queue == EOS_NULL) {
            eos.e_queue = data;
            data->next = EOS_NULL;
            data->last = EOS_NULL;
        }
        else {
            eos_event_data_t *edata = eos.e_queue;
            while (edata->next != EOS_NULL) {
                edata = edata->next;
            }
            data->next = EOS_NULL;
            edata->next = data;
            data->last = edata;
        }
    }
    else if (e_type == EOS_EVENT_ATTRIBUTE_VALUE) {
        eos_event_data_t *data;
        // 挂到Hash表里去
        eos_event_data_t *list = eos.hash.object[e_id].ocb.event.list;
        if (list == EOS_NULL) {
            // Apply one data for the event.
            data = eos_heap_malloc(&eos.heap, sizeof(eos_event_data_t));
            eos.hash.object[e_id].ocb.event.list = data;
            list = data;
            list->owner = owner;
            
            // 挂到事件队列的后面。
            if (eos.e_queue == EOS_NULL) {
                eos.e_queue = data;
                data->next = EOS_NULL;
                data->last = EOS_NULL;
            }
            else {
                eos_event_data_t *edata = eos.e_queue;
                while (edata->next != EOS_NULL) {
                    edata = edata->next;
                }
                data->next = EOS_NULL;
                edata->next = data;
                data->last = edata;
            }
        }
        else {
            list->owner |= owner;
        }
        // Set the event's value.
        for (uint32_t i = 0; i < eos.hash.object[e_id].size; i ++) {
            ((uint8_t *)(eos.hash.object[e_id].data.value))[i] = ((uint8_t *)memory)[i];
        }
    }
    else {
        eos_event_data_t *data;
        // 挂到Hash表里去
        eos_event_data_t *list = eos.hash.object[e_id].ocb.event.list;
        if (list == EOS_NULL) {
            // Apply one data for the event.
            data = eos_heap_malloc(&eos.heap, sizeof(eos_event_data_t));
            eos.hash.object[e_id].ocb.event.list = data;
            list = data;
            
            // 挂到事件队列的后面。
            if (eos.e_queue == EOS_NULL) {
                eos.e_queue = data;
                data->next = EOS_NULL;
                data->last = EOS_NULL;
            }
            else {
                eos_event_data_t *edata = eos.e_queue;
                while (edata->next != EOS_NULL) {
                    edata = edata->next;
                }
                data->next = EOS_NULL;
                edata->next = data;
                data->last = edata;
            }
        }
        list->owner = eos.hash.object[e_id].ocb.event.sub;
        // Push all data to the queue.
        // Check if the remaining memory is enough or not.
        eos_stream_t *queue = eos.hash.object[e_id].data.stream;
        uint32_t size_remain = eos_stream_empty_size(queue);
        EOS_ASSERT(size_remain >= size);
        eos_stream_push(queue, (void *)memory, size);
    }
    eos_critical_exit();

    return (int8_t)EosRun_OK;
}

void eos_event_send_topic(const char *task, const char *topic)
{
    __eos_event_give(task, EosEventGiveType_Send, topic, (void *)0, 0);
    if (eos_current == &task_idle) {
        eos_sheduler();
    }
}

void eos_event_send_stream(const char *topic, void const *data, uint32_t size)
{
    __eos_event_give(EOS_NULL, EosEventGiveType_Send, topic, data, size);
    if (eos_current == &task_idle) {
        eos_sheduler();
    }
}

void eos_event_send_value(const char *task, const char *topic, void const *data)
{
    __eos_event_give(task, EosEventGiveType_Send, topic, data, 0);
    if (eos_current == &task_idle) {
        eos_sheduler();
    }
}

void eos_event_set_value(const char *topic, void *data)
{
    eos_critical_enter();
    
    uint16_t e_id = eos_hash_get_index(topic);
    EOS_ASSERT(e_id != EOS_MAX_OBJECTS);
    
    uint32_t size = eos.hash.object[e_id].size;
    memcpy(eos.hash.object[e_id].data.value, data, size);
    
    eos_critical_exit();
}

void eos_event_pub_topic(const char *topic)
{
    __eos_event_give(EOS_NULL, EosEventGiveType_Publish, topic, EOS_NULL, 0);
    
    if (eos_current == &task_idle) {
        eos_sheduler();
    }
}

void eos_event_pub_value(const char *topic, void *data)
{
    __eos_event_give(EOS_NULL, EosEventGiveType_Publish, topic, data, 0);
    
    if (eos_current == &task_idle) {
        eos_sheduler();
    }
}

static inline void __eos_event_sub(eos_task_t *const me, const char *topic)
{
    eos_critical_enter();
    // 通过Topic找到对应的Object
    uint16_t index;
    index = eos_hash_get_index(topic);
    if (index == EOS_MAX_OBJECTS) {
        index = eos_hash_insert(topic);
        eos.hash.object[index].type = EosObj_Event;
        eos.hash.object[index].attribute = EOS_EVENT_ATTRIBUTE_TOPIC;
    }
    else {
        EOS_ASSERT(eos.hash.object[index].type == EosObj_Event);
        EOS_ASSERT(eos.hash.object[index].attribute != EOS_EVENT_ATTRIBUTE_STREAM);
    }
    // 写入订阅
    eos.hash.object[index].ocb.event.sub |= (1 << me->priority);
    eos_critical_exit();
}

void eos_event_sub(const char *topic)
{
    __eos_event_sub(eos_current, topic);
}

void eos_event_unsub(const char *topic)
{
    eos_critical_enter();
    // 通过Topic找到对应的Object
    uint16_t index = eos_hash_get_index(topic);
    EOS_ASSERT(index != EosObj_Event);
    EOS_ASSERT(eos.hash.object[index].type == EosObj_Event);
    EOS_ASSERT(eos.hash.object[index].attribute != EOS_EVENT_ATTRIBUTE_STREAM);
    // 删除订阅
    eos.hash.object[index].ocb.event.sub &=~ (1 << eos_current->priority);
    eos_critical_exit();
}

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

static inline bool __eos_event_recieve( eos_event_t *const e,
                                        void *buffer, uint32_t size,
                                        uint32_t *size_out)
{
    eos_critical_enter();
    uint8_t priority = eos_current->priority;
    uint32_t bits = (1 << priority);
    EOS_ASSERT((eos.owner_global & bits) != 0);
    
    // Event ID
    uint16_t e_id = ((eos_event_data_t *)(~e->eid))->id;
    if (e_id == EOS_MAX_OBJECTS) {
        eos_critical_exit();
        return false;
    }
    uint8_t type = eos.hash.object[e_id].type;
    
    // Get the data
    eos_event_data_t *data = eos.e_queue;
    while (data != EOS_NULL) {
        // If the event data is NOT belong to the current task.
        if ((data->owner & bits) != 0) {
            continue;
        }
        // Get the data size and copy the data to buffer
        int32_t size_data;
        if (type == EOS_EVENT_ATTRIBUTE_VALUE) {
            size_data = eos.hash.object[e_id].size;
            memcpy(buffer, eos.hash.object[e_id].data.value, size_data);
            *size_out = size_data;
        }
        else if (type == EOS_EVENT_ATTRIBUTE_STREAM) {
            eos_stream_t *queue;
            queue = eos.hash.object[e_id].data.stream;
            size_data = eos_stream_pull_pop(queue, buffer, size);
            EOS_ASSERT(size_data > 0);
            *size_out = size_data;
        }
        
        // If the event data is just the current task's event.
        data->owner &=~ bits;
        if (data->owner == 0) {
            // Delete the event data from the e-queue.
            __eos_e_queue_delete(data);
            // free the event data.
            eos_heap_free(&eos.heap, data);
            // update the global owner flag.
            eos.owner_global = 0;
            eos_event_data_t *e_queue = eos.e_queue;
            while (e_queue != EOS_NULL) {
                eos.owner_global |= e_queue->owner;
                e_queue = e_queue->next;
            }
        }
        eos_critical_exit();
        return true;
    }
    /*  That's impossible that event data can not be found if eos_task_wait_event
     *  or eos_task_wait_specific_event be used in the right way.
     */
    EOS_ASSERT(0);
    eos_critical_exit();
    
    return false;
}

bool eos_event_topic(eos_event_t *const e, const char *topic)
{
    if (strcmp(e->topic, topic) == 0) {
        eos_event_data_t *e_item = (eos_event_data_t *)(~e->eid);
        e_item->owner &=~ (1 << eos_current->priority);
        if (e_item->owner == 0) {
            __eos_e_queue_delete(e_item);
        }
        return true;
    }
    else {
        return false;
    }
}

bool eos_event_value_recieve(eos_event_t *const e, void *value)
{
    return __eos_event_recieve(e, value, 0, EOS_NULL);
}

int32_t eos_event_stream_recieve(eos_event_t *const e, void *buffer, uint32_t size)
{
    uint32_t size_out;
    bool ret = __eos_event_recieve(e, buffer, size, &size_out);
    if (ret == false) {
        return -1;
    }
    else {
        return size_out;
    }
}

void eos_event_get_value(const char *topic, void *value)
{
    eos_critical_enter();
    uint16_t e_id = eos_hash_get_index(topic);
    EOS_ASSERT(e_id != EOS_MAX_OBJECTS);

    eos_object_t *object = &eos.hash.object[e_id];
    memcpy(value, object->data.value, object->size);
    eos_critical_exit();
}

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

/* -----------------------------------------------------------------------------
Heap Library
----------------------------------------------------------------------------- */
// api -------------------------------------------------------------------------
void eos_heap_init(eos_heap_t * const me)
{
    eos_heap_block_t * block_1st;
    
    // block start
    me->list = (eos_heap_block_t *)me->data;
    
    // the 1st free block
    block_1st = (eos_heap_block_t *)me->data;
    block_1st->next = NULL;
    block_1st->size = EOS_SIZE_HEAP - (uint32_t)sizeof(eos_heap_block_t);
    block_1st->is_free = 1;

    // info
    me->size = EOS_SIZE_HEAP;

    me->error_id = 0;
}

void * eos_heap_malloc(eos_heap_t * const me, uint32_t size)
{
    eos_heap_block_t * block;
    
    if (size == 0) {
        me->error_id = 1;
        return NULL;
    }

    /* Find the first free block in the block-list. */
    block = (eos_heap_block_t *)me->list;
    do {
        if (block->is_free == 1 && block->size > (size + sizeof(eos_heap_block_t))) {
            break;
        }
        else {
            block = block->next;
        }
    } while (block != NULL);

    if (block == NULL) {
        me->error_id = 2;
        return NULL;
    }

    /* If the block's byte number is NOT enough to create a new block. */
    if (block->size <= (uint32_t)((uint32_t)size + sizeof(eos_heap_block_t))) {
        block->is_free = 0;
    }
    /* Divide the block into two blocks. */
    else {
        eos_heap_block_t * new_block = (eos_heap_block_t *)((uint32_t)block + size + sizeof(eos_heap_block_t));
        new_block->size = block->size - size - sizeof(eos_heap_block_t);
        new_block->is_free = 1;
        new_block->next = NULL;

        /* Update the list. */
        new_block->next = block->next;
        block->next = new_block;
        block->size = size;
        block->is_free = 0;
    }

    me->error_id = 0;
    return (void *)((uint32_t)block + (uint32_t)sizeof(eos_heap_block_t));
}

void eos_heap_free(eos_heap_t * const me, void * data)
{
    eos_heap_block_t * block_crt = (eos_heap_block_t *)((uint32_t)data - sizeof(eos_heap_block_t));

    /* Search for this block in the block-list. */
    eos_heap_block_t * block = me->list;
    eos_heap_block_t * block_last = NULL;
    do {
        if (block->is_free == 0 && block == block_crt) {
            break;
        }
        else {
            block_last = block;
            block = block->next;
        }
    } while (block != NULL);

    /* Not found. */
    if (block == NULL) {
        me->error_id = 4;
        return;
    }

    block->is_free = 1;
    /* Check the block can be combined with the front one. */
    if (block_last != (eos_heap_block_t *)NULL && block_last->is_free == 1) {
        block_last->size += (block->size + sizeof(eos_heap_block_t));
        block_last->next = block->next;
        block = block_last;
    }
    /* Check the block can be combined with the later one. */
    if (block->next != (eos_heap_block_t *)EOS_NULL && block->next->is_free == 1) {
        block->size += (block->next->size + (uint32_t)sizeof(eos_heap_block_t));
        block->next = block->next->next;
        block->is_free = 1;
    }

    me->error_id = 0;
}

/* -----------------------------------------------------------------------------
Hash
----------------------------------------------------------------------------- */
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

static bool eos_hash_existed(const char *string)
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
            
            return true;
        }

        // 确保哈希表的查找速度
        if (i >= EOS_MAX_HASH_SEEK_TIMES) {
            return false;
        }
    }
    
    return false;
}

/* -----------------------------------------------------------------------------
Stream library
----------------------------------------------------------------------------- */
static int32_t eos_stream_init(eos_stream_t *const me, void *memory, uint32_t capacity)
{
    me->data = (void *)((uint32_t)memory + 4);
    me->capacity = capacity;
    me->head = 0;
    me->tail = 0;
    me->empty = true;

    *((uint32_t *)((uint32_t)me->data - 4)) = EOS_MAGIC;
    *((uint32_t *)((uint32_t)me->data + me->capacity)) = EOS_MAGIC;

    return Stream_OK;
}

static int32_t eos_stream_push(eos_stream_t *const me, void * data, uint32_t size)
{
    if (eos_stream_full(me))
        return Stream_Full;
    if (eos_stream_empty_size(me) < size)
        return Stream_NotEnough;

    for (int i = 0; i < size; i ++)
        ((uint8_t *)me->data)[(me->head + i) % me->capacity] = ((uint8_t *)data)[i];
    me->head = (me->head + size) % me->capacity;
    me->empty = false;

    if (*((uint32_t *)((uint32_t)me->data - 4)) != EOS_MAGIC)
        return Stream_MemCovered;
    if (*((uint32_t *)((uint32_t)me->data + me->capacity)) != EOS_MAGIC)
        return Stream_MemCovered;

    return Stream_OK;
}

static int32_t eos_stream_pull_pop(eos_stream_t *const me, void * data, uint32_t size)
{
    if (me->empty)
        return 0;

    uint32_t size_stream = eos_stream_size(me);
    size = (size_stream < size) ? size_stream : size;

    for (int i = 0; i < size; i ++)
        ((uint8_t *)data)[i] = ((uint8_t *)me->data)[(me->tail + i) % me->capacity];
    
    me->tail = (me->tail + size) % me->capacity;
    if (me->tail == me->head) {
        me->tail = 0;
        me->head = 0;
        me->empty = true;
    }

    return size;
}

static bool eos_stream_full(eos_stream_t *const me)
{
    int size = me->head - me->tail;
    if (size < 0)
        size += me->capacity;

    return (size == 0 && me->empty == false) ? true : false;
}

static int32_t eos_stream_size(eos_stream_t *const me)
{
    if (me->empty == true)
        return 0;

    int size = me->head - me->tail;
    if (size <= 0)
        size += me->capacity;

    return size;
}

static int32_t eos_stream_empty_size(eos_stream_t *const me)
{
    return me->capacity - eos_stream_size(me);
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
