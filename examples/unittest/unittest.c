#include "eventos.h"
#include "m_assert.h"
#include "m_debug.h"

M_MODULE_NAME("mewo_ut")

// data struct -----------------------------------------------------------------
typedef struct led_tag {
    eos_sm_t super;
    bool is_on;
    int count;
    int count_hook1;
    int count_hook2;
} led_t;

// state -----------------------------------------------------------------------
static eos_ret_t led_state_init(led_t * const me, eos_event_t const * const e);
static eos_ret_t led_state_on(led_t * const me, eos_event_t const * const e);
static eos_ret_t led_state_off(led_t * const me, eos_event_t const * const e);

static void led_hook1(eos_sm_t * const me, eos_event_t const * const e);
static void led_hook2(eos_sm_t * const me, eos_event_t const * const e);

// api -------------------------------------------------------------------------
static void led_init(led_t * me, const char* name, int priv, bool en_log)
{
    me->is_on = EOS_False;
    me->count = 0;
    me->count_hook1 = 0;
    me->count_hook2 = 0;

    eos_sm_init((eos_sm_t *)me, name, priv, 200);
    eos_sm_start((eos_sm_t *)me, STATE_CAST(led_state_init));

    m_dbg_mod_enable(EOS_True, name);
}

static int led_get_evt_count(led_t * me)
{
    return me->count;
}

static void led_reset_evt_count(led_t * me)
{
    me->count = 0;
}

// state function --------------------------------------------------------------
static eos_ret_t led_state_init(led_t * const me, eos_event_t const * const e)
{
    (void)e;

    EVT_SUB(Evt_Time_500ms);
    EVT_SUB(Evt_Test1);
    EVT_SUB(Evt_Test2);
    evt_publish_period(Evt_Time_500ms, 500);

    return EOS_TRAN(led_state_off);
}

static eos_ret_t led_state_off(led_t * const me, eos_event_t const * const e)
{
    switch (e->sig) {
        case Evt_Enter:
            M_ERROR("-------------------, Enter");
            me->is_on = EOS_False;
            return M_Ret_Handled;

        case Evt_Time_500ms:
            me->count ++;
            return EOS_TRAN(led_state_on);

        default:
            return EOS_SUPER(eos_state_top);
    }
}

static eos_ret_t led_state_on(led_t * const me, eos_event_t const * const e)
{
    switch (e->sig) {
        case Evt_Enter: {
            M_ERROR("--------------------, Enter");
            int num = 1;
            me->is_on = EOS_True;
            return M_Ret_Handled;
        }

        case Evt_Exit:
            M_ERROR("++++++++++++++++++++, Exit");
            return M_Ret_Handled;

        case Evt_Time_500ms:
            me->count ++;
            return EOS_TRAN(led_state_off);

        default:
            return EOS_SUPER(eos_state_top);
    }
}

static void led_hook1(eos_sm_t * const me, eos_event_t const * const e)
{
    ((led_t *)me)->count_hook1 ++;
}

static void led_hook2(eos_sm_t * const me, eos_event_t const * const e)
{
    ((led_t *)me)->count_hook2 ++;
}


/* unittest ----------------------------------------------------------------- */

typedef struct eos_event_timer_tag {
    int sig;
    int time_ms_delay;
    eos_u32_t timeout_ms;
    bool is_one_shoot;
} eos_event_timer_t;

typedef struct frame_tag {
    eos_u32_t magic_head;
    eos_u32_t evt_sub_tab[Evt_Max];                          // 事件订阅表

    // 状态机池
    eos_s32_t flag_obj_exist;
    eos_s32_t flag_obj_enable;
    eos_sm_t * p_obj[M_SM_NUM_MAX];

    // 关于事件池
    eos_event_t e_pool[M_EPOOL_SIZE];                           // 事件池
    eos_u32_t flag_epool[M_EPOOL_SIZE / 32 + 1];             // 事件池标志位

    // 定时器池
    eos_event_timer_t e_timer_pool[M_ETIMERPOOL_SIZE];
    eos_u32_t flag_etimerpool[M_ETIMERPOOL_SIZE / 32 + 1];   // 事件池标志位
    eos_bool_t is_etimerpool_empty;
    eos_u32_t timeout_ms_min;

    eos_bool_t is_enabled;
    eos_bool_t is_running;
    eos_bool_t is_idle;
    
    eos_u32_t time_crt_ms;
    eos_u32_t magic_tail;
} frame_t;

extern int meow_once(void);
extern void * meow_get_frame(void);
extern int meow_evttimer(void);
extern void set_time_ms(uint64_t time_ms);

#include "stdio.h"
int meow_unittest_sm(void)
{
    m_dbg_init();
    m_dbg_enable(EOS_True);
    m_dbg_flush_enable(EOS_True);
    m_dbg_level(MLogLevel_Print);

    frame_t * f = (frame_t *)meow_get_frame();
    printf("sizeof frame_t: %u.\n", sizeof(frame_t));
    // -------------------------------------------------------------------------
    // 01 meow_init
    M_ASSERT(meow_once() == 1);
    // 检查事件池标志位，事件定时器标志位和事件申请区的标志位。
    eventos_init();
    M_ASSERT(f->is_etimerpool_empty == EOS_True);

    uint32_t _flag_epool[M_EPOOL_SIZE / 32 + 1] = {
        0, 0, 0, 0, 0xfffffffc
    };
    M_ASSERT_MEM(f->flag_epool, _flag_epool, ((M_EPOOL_SIZE / 32 + 1) * 4));
    uint32_t _flag_etimerpool[M_ETIMERPOOL_SIZE / 32 + 1] = {
        0, 0, 0xffffffc0
    };
    printf("f->is_etimerpool_empty: %d.\n", f->is_etimerpool_empty);
    M_ASSERT(f->is_etimerpool_empty == EOS_True);
    M_ASSERT_MEM(f->flag_etimerpool, _flag_etimerpool, ((M_ETIMERPOOL_SIZE / 32 + 1) * 4));
    M_ASSERT(f->is_enabled == EOS_True);
    M_ASSERT(f->is_idle == EOS_True);

    // -------------------------------------------------------------------------
    // 02 meow_stop
    M_ASSERT(meow_once() == 201);
    eventos_stop();
    M_ASSERT(meow_once() == 1);
    M_ASSERT(meow_evttimer() == 210);

    // -------------------------------------------------------------------------
    // 03 sm_init sm_start
    eventos_init();

    static led_t led_test, led2;
    led_init(&led_test, "LedTest", 1, EOS_True);
    led_init(&led2, "Led2", 0, EOS_True);
    M_ASSERT(f->is_etimerpool_empty == EOS_False);
    for (int i = 0; i < 10; i ++)
        M_ASSERT(meow_once() == 202);

    // -------------------------------------------------------------------------
    // 04 sm_init sm_start
    evt_publish_delay(Evt_Time_500ms, 0);
    // 检查事件池

    M_ASSERT(f->flag_epool[0] == 1);
    // 检查每个状态机的事件队列
    M_ASSERT(led_test.super.is_equeue_empty == EOS_False);
    M_ASSERT(led_test.super.head == 1);
    M_ASSERT(led_test.super.tail == 0);
    M_ASSERT(led2.super.is_equeue_empty == EOS_False);
    M_ASSERT(led2.super.head == 1);
    M_ASSERT(led2.super.tail == 0);
    // 优先级高的状态机，消费掉此事件。
    M_ASSERT(meow_once() == 0);
    M_ASSERT(led_test.super.is_equeue_empty == EOS_False);
    M_ASSERT(led_test.super.head == 1);
    M_ASSERT(led_test.super.tail == 0);
    M_ASSERT(led2.super.is_equeue_empty == EOS_True);
    M_ASSERT(led2.super.head == 0);
    M_ASSERT(led2.super.tail == 0);
    // 优先级低的状态机，消费掉此事件。
    M_ASSERT(meow_once() == 0);
    M_ASSERT(led_test.super.is_equeue_empty == EOS_True);
    M_ASSERT(led_test.super.head == 0);
    M_ASSERT(led_test.super.tail == 0);
    M_ASSERT(led2.super.is_equeue_empty == EOS_True);
    M_ASSERT(led2.super.head == 0);
    M_ASSERT(led2.super.tail == 0);
    M_ASSERT(meow_once() == 203);
    M_ASSERT(meow_once() == 202);
    M_ASSERT(f->flag_epool[0] == 0);

    // -------------------------------------------------------------------------
    // 04 evt_publish evt_new
    EVT_PUB(Evt_Time_500ms);
    M_ASSERT(f->flag_epool[0] == 1);
    M_ASSERT(meow_once() == 0);
    M_ASSERT(meow_once() == 0);
    M_ASSERT(meow_once() == 203);
    M_ASSERT(meow_once() == 202);
    M_ASSERT(f->flag_epool[0] == 0);

    int evt_num = M_EPOOL_SIZE - 1;
    // 测试事件池满的情形
    for (int i = 0; i < evt_num; i ++) {
        EVT_PUB(Evt_Time_500ms);
        M_ASSERT_ID(i, f->flag_epool[i / 32] & (1 << (i % 32)) != 0);
        M_ASSERT(led2.super.head == (i + 1));
        M_ASSERT(*(led2.super.e_queue + i) == i);
    }

    // 事件池满，测试通过
    // EVT_PUB(Evt_Time_500ms);
    // 优先级高的状态机执行其事件。
    for (int i = 0; i < evt_num; i ++) {
        M_ASSERT(meow_once() == 0);
    }

    // 优先级低的状态机执行其事件。
    for (int i = 0; i < evt_num; i ++) {
        M_ASSERT(meow_once() == 0);
        M_ASSERT((f->flag_epool[i / 32] & (1 << (i % 32))) == 0);
    }
    M_ASSERT(meow_once() == 203);
    M_ASSERT(meow_once() == 202);
    M_ASSERT_MEM(f->flag_epool, _flag_epool, ((M_EPOOL_SIZE / 32 + 1) * 4));

    // -------------------------------------------------------------------------
    // 05 evt_unsubscribe
    EVT_PUB(Evt_Time_500ms);
    EVT_PUB(Evt_Time_500ms);
    evt_unsubscribe(&led2.super, Evt_Time_500ms);
    M_ASSERT(meow_once() == 204);
    M_ASSERT(meow_once() == 204);
    M_ASSERT(meow_once() == 0);
    M_ASSERT(meow_once() == 0);
    M_ASSERT(meow_once() == 203);
    M_ASSERT(meow_once() == 202);
    M_ASSERT(f->flag_epool[0] == 0);

    // -------------------------------------------------------------------------
    // 06 evt_publish_delay
    evt_subscribe(&led2.super, Evt_Time_500ms);
    M_ASSERT(f->flag_etimerpool[0] == 1);
    M_ASSERT(f->is_etimerpool_empty == EOS_False);
    M_ASSERT(f->e_timer_pool[0].timeout_ms == 500);
    set_time_ms(200);
    M_ASSERT(meow_evttimer() == 211);
    set_time_ms(500);
    M_ASSERT(meow_evttimer() == 0);
    M_ASSERT(f->time_crt_ms == 500);
    M_ASSERT(f->e_timer_pool[0].timeout_ms == 1000);
    set_time_ms(1000);
    M_ASSERT(meow_evttimer() == 0);
    M_ASSERT(f->time_crt_ms == 1000);
    M_ASSERT(f->e_timer_pool[0].timeout_ms == 1500);
    M_ASSERT(f->flag_epool[0] == 3);
    M_ASSERT(meow_once() == 0);
    M_ASSERT(meow_once() == 0);
    M_ASSERT(meow_once() == 0);
    M_ASSERT(meow_once() == 0);
    M_ASSERT(meow_once() == 203);
    M_ASSERT(meow_once() == 202);

    // 检查重复时间定时器的重复设定
    // evt_publish_period(Evt_Time_500ms, 200);

    int count_etimer = (M_ETIMERPOOL_SIZE - 1);
    // 检查时间定时器满
    for (int i = 0; i < count_etimer; i ++) {
        evt_publish_delay(Evt_Time_500ms, 200);
    }
    // evt_publish_delay(Evt_Time_500ms, 200);

    // 检查事件定时器发送出的延时事件的执行
    set_time_ms(1200);
    led_reset_evt_count(&led_test);
    led_reset_evt_count(&led2);
    for (int i = 0; i < count_etimer; i ++) {
        M_ASSERT(meow_once() == 0);
    }
    for (int i = 0; i < count_etimer; i ++) {
        M_ASSERT(meow_once() == 0);
    }
    M_ASSERT(meow_once() == 203);
    M_ASSERT(meow_once() == 202);
    M_ASSERT(led_get_evt_count(&led_test) == count_etimer);
    M_ASSERT(led_get_evt_count(&led2) == count_etimer);

    // 周期延时事件的执行
    set_time_ms(1500);
    led_reset_evt_count(&led_test);
    led_reset_evt_count(&led2);
    // 一个状态机的执行
    M_ASSERT(meow_once() == 0);
    M_ASSERT(led_get_evt_count(&led_test) == 0);
    M_ASSERT(led_get_evt_count(&led2) == 1);
    // 另一个状态机的执行
    M_ASSERT(meow_once() == 0);
    M_ASSERT(meow_once() == 203);
    M_ASSERT(meow_once() == 202);
    M_ASSERT(led_get_evt_count(&led_test) == 1);
    M_ASSERT(led_get_evt_count(&led2) == 1);
    M_ASSERT(f->time_crt_ms == 1500);
    M_ASSERT(f->e_timer_pool[0].timeout_ms == 2000);
    M_ASSERT(f->flag_epool[0] == 0);

    m_print("ALL UNITTEST END!!!! ============================================");
    M_ASSERT(meow_once() == 202);
    
    return 0;
}
