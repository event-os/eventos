
// include ---------------------------------------------------------------------
#include "eventos.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// todo ------------------------------------------------------------------------
// TODO 事件带参的一般化实现
// TODO 可以考虑使用链表实现时间定时器，可以实现无限多个，而且遍历会更加快速。
// TODO 状态机也考虑用链表管理，可以突破数量的限制。最高优先级为0。
// TODO 增加平面状态机模式。
// TODO 增加延时或周期执行回调函数的功能。
// TODO 增加对Shell在内核中的无缝集成。
// TODO 可以考虑对malloc和free进行实现，使事件的参数改为可变长参数机制。
// TODO 添加返回历史状态的功能
// TODO 增加进入低功耗状态的功能
// TODO 优化事件的存储机制，以便减少系统开销，增强健壮性。
//      01  由于携带参数的事件较少，使事件参数额外存储，不跟随事件本身。
//      02  将事件直接存在事件队列里，而非存储事件池ID。开销较大，而且不便于保持稳定。
//      03  增加evt_send_self，用于发送给自身，直接存入事件队列，不再经过事件池。
//      04  事件的订阅机制。
// TODO 当系统ms数字发生变化时，才进行定时器的扫描。
// TODO 定时事件，发送时返回其ID，以便可以取消此定时事件。

// eoswork -------------------------------------------------------------------
typedef struct eos_event_timer_tag {
    eos_s32_t topic;
    eos_s32_t time_ms_delay;
    eos_u32_t timeout_ms;
    eos_bool_t is_one_shoot;
} eos_event_timer_t;

typedef struct eos_tag {
    eos_u32_t magic;
    eos_u32_t *evt_sub_tab;                                     // 事件订阅表

    // 状态机池
    eos_s32_t flag_obj_exist;
    eos_s32_t flag_obj_enable;
    eos_sm_t * sm[EOS_SM_NUM_MAX];

    // 关于事件池
    eos_event_t e_pool[EOS_EPOOL_SIZE];                         // 事件池
    eos_u32_t flag_epool[EOS_EPOOL_SIZE / 32 + 1];              // 事件池标志位

    // 定时器池
    eos_event_timer_t e_timer_pool[EOS_ETIMERPOOL_SIZE];
    eos_u32_t flag_etimerpool[EOS_ETIMERPOOL_SIZE / 32 + 1];    // 事件池标志位
    eos_bool_t etimerpool_empty;
    eos_u32_t timeout_ms_min;

    eos_bool_t enabled;
    eos_bool_t running;
    eos_bool_t idle;
    
    eos_u32_t time_crt_ms;
} eos_t;

static eos_t eos;

#define EOS_MAGIC                         0x4F2EA035

// data ------------------------------------------------------------------------
static const eos_event_t eos_event_table[Event_User] = {
    {Event_Null, 0},
    {Event_Enter, 0},
    {Event_Exit, 0},
    {Event_Init, 0},
};

// macro -----------------------------------------------------------------------
#define HSM_TRIG_(state_, topic_)                                              \
    ((*(state_))(me, &eos_event_table[topic_]))

#define HSM_EXIT_(state_) do { HSM_TRIG_(state_, Event_Exit); } while (0)
#define HSM_ENTER_(state_) do { HSM_TRIG_(state_, Event_Enter); } while (0)

// static function -------------------------------------------------------------
static void eos_sm_dispath(eos_sm_t * const me, eos_event_t const * const e);
static eos_s32_t eos_sm_tran(eos_sm_t * const me, eos_state_handler path[EOS_MAX_NEST_DEPTH]);

// eventos ---------------------------------------------------------------------
static void eos_clear(void)
{
    // 清空事件池
    for (eos_u32_t i = 0; i < EOS_EPOOL_SIZE / 32 + 1; i ++)
        eos.flag_epool[i] = EOS_U32_MAX;
    for (eos_u32_t i = 0; i < EOS_EPOOL_SIZE; i ++)
        eos.flag_epool[i / 32] &= ~(1 << (i % 32));
    // 清空事件定时器池
    for (eos_u32_t i = 0; i < EOS_ETIMERPOOL_SIZE / 32 + 1; i ++)
        eos.flag_etimerpool[i] = EOS_U32_MAX;
    for (eos_u32_t i = 0; i < EOS_ETIMERPOOL_SIZE; i ++)
        eos.flag_etimerpool[i / 32] &= ~(1 << (i % 32));
    eos.etimerpool_empty = EOS_True;
}

void eventos_init(eos_u32_t *flag_sub)
{
    eos_clear();

    eos.enabled = EOS_True;
    eos.running = EOS_False;
    eos.idle = EOS_True;
    eos.magic = EOS_MAGIC;
    eos.evt_sub_tab = flag_sub;
}

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
    for (eos_u32_t i = 0; i < EOS_ETIMERPOOL_SIZE; i ++) {
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
    for (eos_u32_t i = 0; i < EOS_ETIMERPOOL_SIZE; i ++) {
        if ((eos.flag_etimerpool[i / 32] & (1 << (i % 32))) == 0)
            continue;
        if (min_time_out_ms <= eos.e_timer_pool[i].timeout_ms)
            continue;
        min_time_out_ms = eos.e_timer_pool[i].timeout_ms;
    }
    eos.timeout_ms_min = min_time_out_ms;

    return 0;
}

eos_s32_t eos_once(void)
{
    eos_s32_t ret = 0;

    if (eos.enabled == EOS_False) {
        eos_clear();
        return 1;
    }

    // 检查是否有状态机的注册
    if (eos.flag_obj_exist == 0 || eos.flag_obj_enable == 0) {
        ret = 201;
        return ret;
    }

    eos_evttimer();

    if (eos.idle == EOS_True) {
        ret = 202;
        return ret;
    }
    // 寻找到优先级最高且事件队列不为空的状态机
    eos_sm_t * sm = (eos_sm_t *)0;
    for (eos_u32_t i = 0; i < EOS_SM_NUM_MAX; i ++) {
        if ((eos.flag_obj_exist & (1 << i)) == 0)
            continue;
        EOS_ASSERT(eos.sm[i]->magic == EOS_MAGIC);
        if (eos.sm[i]->equeue_empty == EOS_True)
            continue;
        sm = eos.sm[i];
        break;
    }
    if (sm == (eos_sm_t *)0) {
        eos.idle = EOS_True;
        
        ret = 203;
        return ret;
    }

    // 寻找到最老的事件
    eos_u32_t epool_index = *(sm->e_queue + sm->tail);
    eos_port_critical_enter();
    sm->tail ++;
    sm->tail %= sm->depth;
    if (sm->tail == sm->head) {
        sm->equeue_empty = EOS_True;
        sm->tail = 0;
        sm->head = 0;
    }
    eos_port_critical_exit();
    // 对事件进行执行
    eos_event_t * _e = (eos_event_t *)&eos.e_pool[epool_index];
    if ((eos.evt_sub_tab[_e->topic] & (1 << sm->priv)) != 0) {
        // 执行状态的转换
        eos_sm_dispath(sm, _e);
    }
    else
        ret = 204;

    _e->flag_sub &= ~(1 << (sm->priv));

    // 销毁过期事件与其携带的参数
    if (_e->flag_sub == 0) {
        eos_port_critical_enter();
        eos.flag_epool[epool_index / 32] &= ~(1 << (epool_index % 32));
        eos_port_critical_exit();
    }

    return ret;
}

eos_s32_t eventos_run(void)
{
    EOS_ASSERT(eos.enabled == EOS_True);
    eos_hook_start();
    eos.running = EOS_True;

    while (EOS_True) {
        // 检查Magic
        EOS_ASSERT(eos.magic == EOS_MAGIC);
        eos_s32_t ret = eos_once();
        if ((int)(ret / 100) == 2) {
            eos_hook_idle();
        }
        else if (ret == 1)
            break;
    }

    while (EOS_True) {
        eos_hook_idle();
    }
}

void eventos_stop(void)
{
    eos.enabled = EOS_False;
    eos_hook_stop();
}

// state machine ---------------------------------------------------------------
void eos_sm_init(   eos_sm_t * const me,
                    eos_u32_t priority,
                    void *memory_queue, eos_u32_t queue_size,
                    void *memory_stack, eos_u32_t stask_size)
{
    // 框架需要先启动起来
    EOS_ASSERT(eos.enabled == EOS_True);
    EOS_ASSERT(eos.running == EOS_False);
    // 参数检查
    EOS_ASSERT(me != (eos_sm_t *)0);
    EOS_ASSERT(priority >= 0 && priority < EOS_SM_NUM_MAX);

    me->magic = EOS_MAGIC;
    me->state = (void *)eos_state_top;

    // 防止二次启动
    if (me->enabled == EOS_True)
        return;

    // 检查优先级的重复注册
    EOS_ASSERT((eos.flag_obj_exist & (1 << priority)) == 0);

    // 注册到框架里
    eos.flag_obj_exist |= (1 << priority);
    eos.sm[priority] = me;
    // 状态机   
    me->priv = priority;

    // 事件队列
    eos_port_critical_enter();
    me->e_queue = memory_queue;
    EOS_ASSERT_ID(105, me->e_queue != 0);
    me->head = 0;
    me->tail = 0;
    me->depth = queue_size;
    me->equeue_empty = EOS_True;
    eos_port_critical_exit();
}

void eos_sm_start(eos_sm_t * const me, eos_state_handler state_init)
{
    eos_state_handler path[EOS_MAX_NEST_DEPTH];
    eos_state_handler t = eos_state_top;
    eos_s8_t ip = 0;

    me->state = (void *)state_init;
    me->enabled = EOS_True;
    eos.flag_obj_enable |= (1 << me->priv);

    // 进入初始状态，执行TRAN动作。这也意味着，进入初始状态，必须无条件执行Tran动作。
    eos_ret_t ret = me->state(me, &eos_event_table[Event_Null]);
    EOS_ASSERT(ret == EOS_Ret_Tran);

    // 由初始状态转移，引发的各层状态的进入
    // 每一个循环，都代表着一个Event_Init的执行
    ret = EOS_Ret_Null;
    do {
        // 由当前层，探测需要进入的各层父状态
        path[0] = me->state;
        // 一层一层的探测，一直探测到原状态
        HSM_TRIG_(me->state, Event_Null);
        while (me->state != t) {
            ++ ip;
            EOS_ASSERT(ip < EOS_MAX_NEST_DEPTH);
            path[ip] = me->state;
            HSM_TRIG_(me->state, Event_Null);
        }
        me->state = (void *)path[0];

        // 各层状态的进入
        do {
            HSM_ENTER_(path[ip --]);
        } while (ip >= 0);

        t = path[0];

        ret = HSM_TRIG_(t, Event_Init);
    } while (ret == EOS_Ret_Tran);

    me->state = (void *)t;
}

// event -----------------------------------------------------------------------
void eos_event_pub_topic(eos_topic_t topic)
{
    eos_u32_t para[EOS_EVENT_PARAS_NUM];
    eos_event_pub(topic, para, EOS_EVENT_PARAS_NUM);
}

void eos_event_pub(eos_topic_t topic, void *data, eos_u32_t size)
{
    // 保证框架已经运行
    EOS_ASSERT(eos.enabled == EOS_True);
    // 保证参数不大于最大支持数
    EOS_ASSERT(size <= EOS_EVENT_PARAS_NUM);
    // 没有状态机使能，返回
    if (eos.flag_obj_enable == 0)
        return;
    // 没有状态机订阅，返回
    if (eos.evt_sub_tab[topic] == 0)
        return;
    // 新建事件
    static eos_event_t e;
    e.topic = topic;
    e.flag_sub = eos.evt_sub_tab[topic];
    for (eos_u32_t i = 0; i < size; i ++)
        e.para[i] = ((eos_u8_t *)data)[i];
    // 如果事件池满，进入断言；如果未满，获取到空位置，并将此位置1
    eos_port_critical_enter();
    eos_s32_t index_empty = EOS_U32_MAX;
    for (eos_u32_t i = 0; i < (EOS_EPOOL_SIZE / 32 + 1); i ++) {
        if (eos.flag_epool[i] == EOS_U32_MAX)
            continue;
        for (eos_s32_t j = 0; j < 32; j ++) {
            if ((eos.flag_epool[i] & (1 << j)) == 0) {
                eos.flag_epool[i] |= (1 << j);
                index_empty = i * 32 + j;
                break;
            }
        }
        break;
    }
    EOS_ASSERT_ID(101, index_empty != EOS_U32_MAX);
    eos.e_pool[index_empty] = *(eos_event_t *)&e;

    // 根据flagSub的信息，将事件推入各对象
    for (eos_u32_t i = 0; i < EOS_SM_NUM_MAX; i ++) {
        if ((eos.flag_obj_exist & (1 << i)) == 0)
            continue;
        if (eos.sm[i]->enabled == EOS_False)
            continue;
        if ((eos.evt_sub_tab[e.topic] & (1 << i)) == 0)
            continue;
        // 如果事件队列满，进入断言
        if (((eos.sm[i]->head + 1) % eos.sm[i]->depth) == eos.sm[i]->tail)
            EOS_ASSERT_ID(104, EOS_False);
        // 事件队列未满，将事件池序号放入事件队列
        *(eos.sm[i]->e_queue + eos.sm[i]->head) = index_empty;
        
        eos.sm[i]->head ++;
        eos.sm[i]->head %= eos.sm[i]->depth;
        if (eos.sm[i]->equeue_empty == EOS_True)
            eos.sm[i]->equeue_empty = EOS_False;
        eos.idle = EOS_False;
    }
    eos_port_critical_exit();
}

void eos_event_sub(eos_sm_t * const me, eos_topic_t topic)
{
    eos.evt_sub_tab[topic] |= (1 << me->priv);
}

void eos_event_unsub(eos_sm_t * const me, eos_topic_t topic)
{
    eos.evt_sub_tab[topic] &= ~(1 << me->priv);
}

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
        for (eos_u32_t i = 0; i < EOS_ETIMERPOOL_SIZE; i ++) {
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

    // 寻找到空的时间定时器
    eos_s32_t index_empty = EOS_U32_MAX;
    for (eos_u32_t i = 0; i < (EOS_ETIMERPOOL_SIZE / 32 + 1); i ++) {
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
        topic, time_ms, time_crt_ms + time_ms, is_oneshoot
    };

    eos.etimerpool_empty = EOS_False;
    
    // 寻找到最小的时间定时器
    eos_u32_t min_time_out_ms = EOS_U32_MAX;
    for (eos_u32_t i = 0; i < EOS_ETIMERPOOL_SIZE; i ++) {
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

// state tran ------------------------------------------------------------------
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

// static function -------------------------------------------------------------
static void eos_sm_dispath(eos_sm_t * const me, eos_event_t const * const e)
{
    eos_state_handler path[EOS_MAX_NEST_DEPTH];
    eos_state_handler t = me->state;
    eos_state_handler s;
    eos_ret_t r;

    EOS_ASSERT(e != (eos_event_t *)0);

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
        me->state = (void *)t;                                  // 更新当前状态
        me->state = (void *)t;                                  // 防止覆盖
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
    me->state = (void *)t; // update the next state

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
        me->state = (void *)path[0];

        // 层数不能大于MAX_NEST_DEPTH_
        EOS_ASSERT(ip < EOS_MAX_NEST_DEPTH);

        // retrace the entry path in reverse (correct) order...
        do {
            HSM_ENTER_(path[ip --]);                   // 进入path[ip]
        } while (ip >= 0);

        t = path[0];
    }

    me->state = (void *)t;                                  // 更新当前状态
}

static eos_s32_t eos_sm_tran(eos_sm_t * const me, eos_state_handler path[EOS_MAX_NEST_DEPTH])
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
            EOS_ASSERT(ip < EOS_MAX_NEST_DEPTH);
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
        EOS_ASSERT(ip < EOS_MAX_NEST_DEPTH);

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

/* for unittest ------------------------------------------------------------- */
void * eos_get_framework(void)
{
    return (void *)&eos;
}

#ifdef __cplusplus
}
#endif
