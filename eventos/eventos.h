
#ifndef EVENTOS_H_
#define EVENTOS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* include ------------------------------------------------------------------ */
#include "stdint.h"
#include "stdbool.h"
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
    // 局部事件区
    Evt_Apply = Evt_Time_Max,
    Evt_User = (Evt_Apply + EVT_APPLY_SIZE),
};

enum {
    EvtMode_Ps = 0,
    EvtMode_Send = !EvtMode_Ps
};

// 用户事件的定义
#include "evt_def.h"

// 状态返回值的定义
typedef enum m_ret_tag {
    M_Ret_Null = 0,                     // 状态机与行为树共用
    M_Ret_Super,                        // 状态机专用
    M_Ret_Handled,                      // 状态机专用
    M_Ret_Tran,                         // 状态机专用

    M_Ret_Max
} m_ret_t;

// AO类型的定义
typedef enum m_obj_type_tag {
    ObjType_Sm = 0,
    ObjType_BeTree,
} m_obj_type_t;

// 带16字节参数的事件类
typedef struct m_evt_tag {
    int sig;
    unsigned int flag_sub;
    int mode;
    union {
        unsigned int u32;
        int s32;
        unsigned short u16[2];
        short s16[2];
        unsigned char u8[4];
        char s8[4];
        float f32;
    } para[MEOW_EVT_PARAS_NUM];
} m_evt_t;

typedef struct hook_list_tag {
    struct hook_list_tag * next;
    void * hook;
    int evt_id;
} m_hook_list;

typedef struct m_state_log_tag {
    struct m_state_log_tag * next;
    void * state;
    const char * name;
} m_state_log_t;
// 数据结构 - 行为树相关 --------------------------------------------------------
// 状态机模式
typedef struct m_mode_sm_tag {
    volatile void * state_crt;
    volatile void * state_tgt;
    m_state_log_t * state_log_list;
    m_hook_list * hook_list;
} m_mode_sm_t;

// 行为对象类
typedef struct m_obj_tag {
    uint32_t magic_head;
    const char* name;                               // 状态机名称（用于Log打印）
    m_obj_type_t obj_type;
    int priv;
    bool is_enabled;
    bool is_log_enabled;
    // 状态机模式
    m_mode_sm_t * sm;
    // evt queue
    int* e_queue;
    int head;
    int tail;
    int depth;
    bool is_equeue_empty;
    uint32_t magic_tail;
} m_obj_t;

// 状态函数句柄的定义
typedef m_ret_t (* m_state_handler)(m_obj_t * const me, m_evt_t const * const e);

// api -------------------------------------------------------------------------
// 关于Meow框架 ---------------------------------------------
// 对框架进行初始化，在各状态机初始化之前调用。
void meow_init(void);
// 启动框架，放在main函数的末尾。
int meow_run(void);
// 停止框架的运行（不常用）
// 停止框架后，框架会在执行完当前状态机的当前事件后，清空各状态机事件队列，清空事件池，
// 不再执行任何功能，直至框架被再次启动。
void meow_stop(void);

// 关于状态机 -----------------------------------------------
// 状态机初始化函数
void obj_init(m_obj_t * const me, const char* name, m_obj_type_t obj_type, int pri, int fifo_depth);
void sm_start(m_obj_t * const me, m_state_handler state_init);
int sm_reg_hook(m_obj_t * const me, int evt_id, m_state_handler hook);
int sm_state_name(m_obj_t * const me, m_state_handler state, const char * name);
void sm_log_en(m_obj_t * const me, bool log_en);

// 关于事件 -------------------------------------------------
void evt_name(int evt_id, const char * name);
void evt_subscribe(m_obj_t * const me, int e_id);
void evt_unsubscribe(m_obj_t * const me, int e_id);
void evt_unsub_all(m_obj_t * const me);
// 注：只有此函数能在中断服务函数中使用，其他都没有必要。如果使用，可能会导致崩溃问题。
void evt_publish(int evt_id);
void evt_send(const char * obj_name, int evt_id);
void evt_send_para(const char * obj_name, int evt_id, int para[]);
void evt_publish_para(int evt_id, int para[]);

void evt_publish_delay(int evt_id, int time_ms);
void evt_publish_period(int evt_id, int time_ms_period);
int evt_apply(void);
void evt_unapply(int evt_id);

#define EVT_SUB(_evt)               evt_subscribe(&(me->super), _evt)
#define EVT_UNSUB(_evt)             evt_unsubscribe(&(me->super), _evt)
#define EVT_UNSUB_ALL(me)           evt_unsub_all(me)

#define EVT_PUB(_evt)               evt_publish(_evt)
#define EVT_PUB_PARA(_evt, _para)   evt_publish(_evt, _para)

// 关于状态 -------------------------------------------------
m_ret_t m_tran(m_obj_t * const me, m_state_handler state);
m_ret_t m_super(m_obj_t * const me, m_state_handler state);
m_ret_t m_state_top(m_obj_t * const me, m_evt_t const * const e);

#define M_TRAN(target)              m_tran((m_obj_t * )me, (m_state_handler)target)
#define M_SUPER(super)              m_super((m_obj_t * )me, (m_state_handler)super)
#define STATE_CAST(state)           (m_state_handler)(state)

/* log ---------------------------------------------------------------------- */
void meow_log_enable(bool log_enable);
void meow_smf_log_enable(bool smf_log_enable);

/* port --------------------------------------------------------------------- */
uint64_t port_get_time_ms(void);
void port_critical_enter(void);
void port_critical_exit(void);
void * port_malloc(uint32_t size, const char * name);

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
