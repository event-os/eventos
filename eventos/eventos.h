
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
#if (MEOW_BETREE_EN != 0)
    M_Ret_Failed,                       // 行为树专用
    M_Ret_Running,                      // 行为树专用
    M_Ret_Success,                      // 行为树专用
#endif

    M_Ret_Max
} m_ret_t;

#if (MEOW_BETREE_EN != 0)
typedef enum bt_status_tag {
    BtStatus_Idle = 0,
    BtStatus_Sleep,
    BtStatus_Running,
    BtStatus_Failed,
    BtStatus_Success
} bt_status_t;

enum {
    BtfNode_Action = 0,
    BtfNode_Control,
};

enum {
    BtfCtrl_Null = 0,
    BtfCtrl_Sequence,
    BtfCtrl_Parallel,
    BtfCtrl_Select,
};
#endif

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

#if (MEOW_BETREE_EN != 0)
// 数据结构 - 行为树相关 --------------------------------------------------------
// 节点
typedef struct btf_node_tag {
    int node_type;
    void * func;                                    // 节点函数
    void * self_condition;                          // 内部条件
    int ctrl_type;                                  // 控制节点类型
    int para[MEOW_ACT_PARA_SIZE];                   // 行为节点参数
    struct btf_node_tag * child[BETREE_NODE_CHILD_MAX]; // 子节点
    void * child_condition[BETREE_NODE_CHILD_MAX];  // 子节点外部条件
    int count_child;                                // 子节点数量
    int depth;                                      // 控制节点深度
    int time_out_ms;                                // 超时时间
} btf_node_t;

// 行为树
typedef struct betree_tag {
    const char* name;                               // 行为树名称
    int id;
    btf_node_t * root;                              // 根节点
    bt_status_t status;                             // 行为树执行状态
    struct betree_tag * next;                       // 下一个行为树
    int depth;                                      // 行为树深度
} betree_t;
#endif

// 状态机模式
typedef struct m_mode_sm_tag {
    volatile void * state_crt;
    volatile void * state_tgt;
    m_state_log_t * state_log_list;
    m_hook_list * hook_list;
} m_mode_sm_t;

#if (MEOW_BETREE_EN != 0)
// 控制节点的压栈信息
typedef struct m_node_stack_tag {
    btf_node_t * node;
    int index;
    int time_out_ms;                                // 超时时刻
} m_node_stack_t;

// 行为树模式
typedef struct m_mode_betree_tag {
    betree_t * betree_list;                         // 行为树列表
    betree_t * betree_crt;                          // 当前行为树
    bool is_idle;                                   // 没有行为树在执行
    m_node_stack_t stack[MAX_BETREE_DEPTH];         // 控制节点栈
    int count_stack;                                // 栈深度
    int node_id_crt_se;                             // 当前顺序或者选择控制节点下的当前子节点ID
    bool node_act_success[BETREE_NODE_CHILD_MAX];   // 并行控制节点下完成的行为子节点
} m_mode_betree_t;
#endif

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
#if (MEOW_BETREE_EN != 0)
    // 行为树模式
    m_mode_betree_t * bt;
#endif
    // evt queue
    int* e_queue;
    int head;
    int tail;
    int depth;
    bool is_equeue_empty;
    uint32_t magic_tail;
} m_obj_t;

#if (MEOW_BETREE_EN != 0)
// 行为函数句柄的定义
typedef m_ret_t (* m_act_handler)(m_obj_t * const me, m_evt_t const * const e, int para[]);
// 行为树条件函数
typedef bool (* m_condition_t)(m_obj_t * const me);
#endif
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

#if (MEOW_BETREE_EN != 0)
// 关于行为树 -----------------------------------------------
// 行为节点创建
btf_node_t * bt_node_act_create(m_act_handler func, int para[]);
// 控制节点创建
btf_node_t * bt_node_ctrl_create(int ctrl_type);
// 向节点添加条件
void bt_node_set_condition(btf_node_t * const node, m_condition_t condition);
// 节点设置超时时间
void bt_node_set_timeout(btf_node_t * const node, int time_out_ms);
// 向控制节点添加节点
void bt_ctrl_add_node(btf_node_t * const node_ctrl, btf_node_t * const node);
// 向控制节点添加条件节点
void bt_ctrl_add_node_condition(btf_node_t * const node_ctrl,
                                btf_node_t * const node,
                                m_condition_t condition);
// 行为树创建
betree_t * bt_tree_create(const char* name, int id, btf_node_t * const root);
// 向对象添加行为树
void bt_add_betree(m_obj_t * const ao, betree_t * const betree);
// 对象启动
void bt_start(m_obj_t * const me);
// 行为树运行
// 不能同时运行同一个对象的多个行为树。因此需要检查，当前对象下有没有行为树在运行。
void bt_tree_run(const char * obj_name, const char* betree_name);
void bt_tree_run_id(const char * obj_name, int id);
void bt_tree_stop(const char * obj_name, const char* betree_name);
bt_status_t bt_tree_run_status(const char * obj_name, const char* betree_name);
#endif

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
#if (MEOW_BETREE_EN != 0)
#define ACT_CAST(act)               (m_act_handler)(act)
#define CONDITION_CAST(condition)   (m_condition_t)(condition)
#endif

/* log ---------------------------------------------------------------------- */
void meow_log_enable(bool log_enable);
void meow_smf_log_enable(bool smf_log_enable);
#if (MEOW_BETREE_EN != 0)
void meow_btf_log_enable(bool btf_log_enable);
#endif

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
#if (MEOW_BETREE_EN != 0)
int meow_unittest_betree(void);
#endif

#ifdef __cplusplus
}
#endif

#endif

/* 说明 ------------------------------------------------------------------------
01  框架的说明
    此框架包含以下几个要素：节点（行为节点、控制节点）、分支、行为树、对象与框架。
    A 整个框架支持多个对象，而非仅仅对机器人整机作为行为树的主体。
    B 每个对象支持多个行为树，在同一时刻，只有某个行为树在运行。
    C 行为树，由分支来连接各行为节点与控制节点。
    D 行为节点，就是所谓的原子动作。

02  行为树框架采用事件驱动，而事件采用发布订阅机制。

03  行为树框架，完全可以与状态机框架合二为一，框架设置两种模式：
    A 状态机模式
    B 行为树模式

04  行为树的中止操作与根节点
    行为节点，需要设置行为函数，而无需设置分支。
    控制节点，正好相反，需要设置分支，无需设置行为函数。
    根节点，两者都需要设置（除非此行为树只有根节点这一个节点）。原因是，分支为了实现数，
    而行为函数，是为了实现中止操作。
    另：控制节点的行为函数，是否需要设置和实现，需要在实际应用中进一步看看。

05  很多行为树框架中，一般采用黑板机制来实现信息的交互。
    黑板机制，实际上就用指针将需要的信息传递进去。这种方式的缺点是，不够一般化，信息的产
    生和消费，不能够解耦。
    在这里，采用事件机制进行信息的交互。事件机制与状态机框架相同。

06  如何让框架对行为树的行为函数进行执行结束，并执行Evt_Exit，需要在实现中好好思索。

07  选择节点的执行条件，使用函数来实现。虽然这种方法，有可能会导致程序非常臃肿，但选择节
    点估计使用的情况不太多。
    在程序中顺序节点估计使用的情况最多，并行、循环和选择应该都比较少。
    这个问题不能十分确定，要去程序里进行验证。

08  关于状态机与行为树
    A 状态机中的状态函数，就相当于节点函数。不同的是，状态函数之间的跳转关系，是在状态函
      数的实现中体现的，如状态跳转（M_TRAN），状态嵌套（Evt_Init与M_SUPER)。行为树中的
      跳转关系，是在一开始建立行为树就确定了，通过对节点的创建、树的创建、节点之间的关系
      的创建来确定的。运行时，仅仅按照既定的跳转关闭运行即可。
    B 状态机的跳转关系，是没有办法结构化的。也是不太容易继承的。
      而行为树，是完全可以继承的，可复用的。不仅跳转关系可复用，节点也是高度可复用。更重
      要的是，有限的行为节点，会组成无限的行为树（行为策略）。
      对于状态机而言，在无限的行为策略的实现上，只能无限膨胀。
    C 如果行为节点固化并参数化（形成原子动作），最终行为树是可以脚本化的。最终，以文件
      或者图形界面来配置行为策略，是完全可行的。

09  不允许并行节点，添加控制节点。也就是说，并行节点的子节点，必须全部都是行为节点。

    如果并行控制节点下包含控制节点（但不能包含并行控制节点）。
    实现起来，需要开多个栈，对每一个分支的控制节点进行保存。一个控制节点支持几个子节
    点，就需要几个栈。极度占用空间。如果根据需要进行动态申请和释放，又会产生内存的碎
    片化。因此，为了降低实现的难度，并行控制节点下，只能包含行为节点。

10  选择节点，在同一时间，最多只能运行一个节点。只要选择节点的一个子节点执行完成，此选择
    节点，就执行完成。
    
11  根节点的深度为0。

*/
