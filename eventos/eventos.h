
#ifndef EVENTOS_H_
#define EVENTOS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* include ------------------------------------------------------------------ */
#include "eventos_def.h"
#include "eventos_config.h"

/* data struct -------------------------------------------------------------- */
// 系统事件、时间事件与局部事件区的定义
enum m_evt_id_tag {
    // 系统事件
    Evt_Null = 0, Evt_Enter, Evt_Exit, Evt_Init, Evt_DefaultMax,
    Evt_Tick, Evt_SystemMax,
    // 系统时间事件
    Evt_Time_10ms = Evt_SystemMax,
    Evt_Time_50ms,
    Evt_Time_100ms,
    Evt_Time_200ms,
    Evt_Time_500ms,
    Evt_Time_1000ms,
    Evt_Time_2000ms,
    Evt_Time_5000ms,
    Evt_Time_Max,
    Evt_User = Evt_Time_Max,
};

enum {
    EvtMode_Ps = 0,
    EvtMode_Send = !EvtMode_Ps
};

// 用户事件的定义
#include "evt_def.h"

typedef union eos_time {
    eos_u32_t day           : 5;
    eos_u32_t hour          : 5;
    eos_u32_t minute        : 6;
    eos_u32_t second        : 6;
    eos_u32_t ms            : 10;
} eos_time_t;

// 状态返回值的定义
typedef enum eos_ret {
    M_Ret_Null = 0,                     // 状态机与行为树共用
    M_Ret_Super,                        // 状态机专用
    M_Ret_Handled,                      // 状态机专用
    EOS_Ret_Tran,                       // 状态机专用

    M_Ret_Max
} eos_ret_t;

// 带16字节参数的事件类
typedef struct eos_event {
    eos_s32_t sig;
    eos_u32_t flag_sub;
    eos_s32_t mode;
    union {
        eos_u32_t u32;
        eos_s32_t s32;
        eos_u16_t u16[2];
        short s16[2];
        eos_u8_t u8[4];
        char s8[4];
        float f32;
    } para[MEOW_EVT_PARAS_NUM];
} eos_event_t;

typedef struct hook_list_tag {
    struct hook_list_tag * next;
    void * hook;
    eos_s32_t evt_id;
} m_hook_list;

// 数据结构 - 行为树相关 --------------------------------------------------------
// 行为对象类
typedef struct eos_sm_tag {
    eos_u32_t magic_head;
    const char* name;                               // 状态机名称（用于Log打印）
    eos_s32_t priv;
    eos_bool_t is_enabled;
    volatile void * state_crt;
    volatile void * state_tgt;
    // evt queue
    eos_s32_t* e_queue;
    eos_s32_t head;
    eos_s32_t tail;
    eos_s32_t depth;
    eos_bool_t is_equeue_empty;
    eos_u32_t magic_tail;
} eos_sm_t;

// 状态函数句柄的定义
typedef eos_ret_t (* eos_state_handler)(eos_sm_t *const me, eos_event_t const * const e);

// api -------------------------------------------------------------------------
// 关于Meow框架 ---------------------------------------------
// 对框架进行初始化，在各状态机初始化之前调用。
void eventos_init(void);
// 启动框架，放在main函数的末尾。
int eventos_run(void);
// 停止框架的运行（不常用）
// 停止框架后，框架会在执行完当前状态机的当前事件后，清空各状态机事件队列，清空事件池，
// 不再执行任何功能，直至框架被再次启动。
void eventos_stop(void);

// 关于状态机 -----------------------------------------------
// 状态机初始化函数
void eos_sm_init(eos_sm_t * const me, const char* name, eos_s32_t pri, eos_s32_t fifo_depth);
void eos_sm_start(eos_sm_t * const me, eos_state_handler state_init);

// 关于事件 -------------------------------------------------
void evt_subscribe(eos_sm_t * const me, eos_s32_t e_id);
void evt_unsubscribe(eos_sm_t * const me, eos_s32_t e_id);
void evt_unsub_all(eos_sm_t * const me);
// 注：只有此函数能在中断服务函数中使用，其他都没有必要。如果使用，可能会导致崩溃问题。
void evt_publish(int evt_id);
void evt_send(const char * obj_name, eos_s32_t evt_id);
void evt_send_para(const char * obj_name, eos_s32_t evt_id, eos_s32_t para[]);
void evt_publish_para(int evt_id, eos_s32_t para[]);

void evt_publish_delay(int evt_id, eos_s32_t time_ms);
void evt_publish_period(int evt_id, eos_s32_t time_ms_period);

#define EVT_SUB(_evt)               evt_subscribe(&(me->super), _evt)
#define EVT_UNSUB(_evt)             evt_unsubscribe(&(me->super), _evt)
#define EVT_UNSUB_ALL(me)           evt_unsub_all(me)

#define EVT_PUB(_evt)               evt_publish(_evt)
#define EVT_PUB_PARA(_evt, _para)   evt_publish(_evt, _para)

// 关于状态 -------------------------------------------------
eos_ret_t eos_tran(eos_sm_t * const me, eos_state_handler state);
eos_ret_t eos_super(eos_sm_t * const me, eos_state_handler state);
eos_ret_t eos_state_top(eos_sm_t * const me, eos_event_t const * const e);

#define EOS_TRAN(target)            eos_tran((eos_sm_t * )me, (eos_state_handler)target)
#define EOS_SUPER(super)            eos_super((eos_sm_t * )me, (eos_state_handler)super)
#define STATE_CAST(state)           (eos_state_handler)(state)

/* port --------------------------------------------------------------------- */
eos_u32_t port_get_time_ms(void);
void port_critical_enter(void);
void port_critical_exit(void);
void * port_malloc(eos_u32_t size, const char * name);

/* hook --------------------------------------------------------------------- */
void hook_idle(void);
void hook_stop(void);
void hook_start(void);

/* for unittest ------------------------------------------------------------- */
int meow_unittest_sm(void);

#ifdef __cplusplus
}
#endif

#endif
