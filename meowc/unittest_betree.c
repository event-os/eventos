
// include ---------------------------------------------------------------------
#include "meow.h"
#include "stdio.h"
#include "mdebug/m_assert.h"

M_MODULE_NAME("betree_ut")

// object ----------------------------------------------------------------------
typedef struct m_evt_timer_tag {
    int sig;
    int time_ms_delay;
    uint64_t timeout_ms;
    bool is_one_shoot;
} m_evt_timer_t;

typedef struct m_log_tag {
    bool is_enabled;
    int index;
    char buffer[MEOW_LOG_SIZE];
    int count;
    int count_before_enter;
    int level_set;
    int time_ms_period;
    uint64_t time_ms_bkp;
} m_log_t;

typedef struct m_log_time_tag {
    int hour;
    int minute;
    int second;
    int ms;
} m_log_time_t;

typedef struct frame_tag {
    uint32_t evt_sub_tab[Evt_Max];                          // 事件订阅表
#if (MEOW_SYSTEMLOG_EN != 0)
    const char * evt_name[Evt_Max];                         // 事件名称表
#endif

    // 状态机池
    int flag_sm_exist;
    int flag_sm_enable;
    m_obj_t * p_sm[M_SM_NUM_MAX];

    // 关于事件池
    m_evt_t e_pool[M_EPOOL_SIZE];                           // 事件池
    uint32_t flag_epool[M_EPOOL_SIZE / 32 + 1];             // 事件池标志位

    // 定时器池
    m_evt_timer_t e_timer_pool[M_ETIMERPOOL_SIZE];
    uint32_t flag_etimerpool[M_ETIMERPOOL_SIZE / 32 + 1];   // 事件池标志位
    bool is_etimerpool_empty;
    uint64_t timeout_ms_min;

    uint32_t flag_apply[EVT_APPLY_SIZE / 32 + 1];

    bool is_log_enabled;
    bool is_enabled;
    bool is_running;
    bool is_idle;
    
    uint64_t time_crt_ms;
    m_log_time_t log_time;
} frame_t;

typedef struct test_betree_tag {
    m_obj_t super;
    int act_id;

    int count_node1;
    int count_node2;

    int count_node3;
    int count_node4;

    bool end_node1;
    bool end_node2;

    bool end_node3;
    bool end_node4;

    int condition_value;
} test_betree_t;

// act function ----------------------------------------------------------------
static m_ret_t act_node_1(test_betree_t * const me, m_evt_t const * const e, int para[]);
static m_ret_t act_node_2(test_betree_t * const me, m_evt_t const * const e, int para[]);
static m_ret_t act_node_3(test_betree_t * const me, m_evt_t const * const e, int para[]);
static m_ret_t act_node_4(test_betree_t * const me, m_evt_t const * const e, int para[]);

static bool condition_1(test_betree_t * const me);
static bool condition_2(test_betree_t * const me);
static bool condition_3(test_betree_t * const me);
static bool condition_4(test_betree_t * const me);

static void set_condition(test_betree_t * const me, int value);
static void test_betree_clear(test_betree_t * const me);

static m_evt_t * evt_new(int evt_id);

extern int meow_once(void);
extern void * meow_get_frame(void);
extern int meow_evttimer(void);
extern void set_time_ms(uint64_t time_ms);
extern int ut_betree_execute(m_obj_t * const me, m_evt_t const * const e);

// static function -------------------------------------------------------------
static void test_betree_init(test_betree_t * const test_betree,
                             const char * name, int pri, int fifo_depth);

// unittest --------------------------------------------------------------------
static test_betree_t test_betree;
static m_obj_t * obj = (m_obj_t *)0;
int ret_meow_once = 0;
int count_tick = 10000;
int meow_unittest_betree(void)
{
    frame_t * f = (frame_t *)meow_get_frame();
    // -------------------------------------------------------------------------
    // 01 meow_init
    M_ASSERT(meow_once() == 1);
    // 检查事件池标志位，事件定时器标志位和事件申请区的标志位。
    meow_init();

    // -------------------------------------------------------------------------
    // 02 meow_once
    M_ASSERT(meow_once() == 201);
    test_betree_init(&test_betree, "betree", 2, 30);
    M_ASSERT(meow_once() == 201);
    bt_start(obj);
    M_ASSERT(meow_once() == 202);

    EVT_PUB(Evt_Tick);
    M_ASSERT(meow_once() == 0);
    M_ASSERT(meow_once() == 203);
    M_ASSERT(meow_once() == 202);
    M_ASSERT(test_betree.act_id == 0);

    // 行为树1 -----------------------------------------------------------------
    // 运行行为树1，并检查内部数据。
    bt_tree_run("betree", "betree1");
    M_ASSERT(test_betree.act_id == 1);
    M_ASSERT(test_betree.count_node1 == 0);
    M_ASSERT(test_betree.super.bt->node_id_crt_se == 0);

    // 在行为节点1的功能函数中，运行count_tick次Evt_Tick
    for (int i = 0; i < count_tick; i ++) {
        EVT_PUB(Evt_Tick);
        M_ASSERT(meow_once() == 0);
        M_ASSERT(test_betree.count_node1 == 1 + i);
    }

    // 未订阅Evt_ActEnd事件时进行的单元测试
    M_ASSERT(meow_once() == 203);
    EVT_PUB(Evt_ActEnd);
    M_ASSERT(meow_once() == 202);
    M_ASSERT(test_betree.end_node1 == false);
    M_ASSERT(bt_tree_run_status("betree", "betree1") == BtStatus_Running);
    M_ASSERT(bt_tree_run_status("betree", "betree2") == BtStatus_Sleep);

    // 订阅Evt_ActEnd事件时进行的单元测试，当前行为节点完成，进行下一个行为节点
    evt_subscribe(obj, Evt_ActEnd);
    EVT_PUB(Evt_ActEnd);
    M_ASSERT(meow_once() == 0);
    M_ASSERT(test_betree.end_node1 == true);
    M_ASSERT(meow_once() == 203);
    M_ASSERT(meow_once() == 202);
    M_ASSERT(bt_tree_run_status("betree", "betree1") == BtStatus_Running);
    M_ASSERT(bt_tree_run_status("betree", "betree2") == BtStatus_Sleep);
    M_ASSERT(test_betree.act_id == 2);
    M_ASSERT(test_betree.count_node2 == 0);
    M_ASSERT(test_betree.super.bt->node_id_crt_se == 1);
    M_ASSERT(bt_tree_run_status("betree", "betree1") == BtStatus_Running);
    M_ASSERT(bt_tree_run_status("betree", "betree2") == BtStatus_Sleep);

    // 在行为节点2的功能函数中，运行count_tick次Evt_Tick
    for (int i = 0; i < count_tick; i ++) {
        EVT_PUB(Evt_Tick);
        M_ASSERT(meow_once() == 0);
        M_ASSERT(test_betree.count_node2 == 1 + i);
    }
    M_ASSERT(bt_tree_run_status("betree", "betree1") == BtStatus_Running);
    M_ASSERT(bt_tree_run_status("betree", "betree2") == BtStatus_Sleep);

    // 发送Evt_ActEnd，行为节点2完成，行为树完成
    EVT_PUB(Evt_ActEnd);
    M_ASSERT(meow_once() == 0);
    M_ASSERT(test_betree.end_node2 == true);
    M_ASSERT(meow_once() == 203);
    M_ASSERT(meow_once() == 202);
    M_ASSERT(bt_tree_run_status("betree", "betree1") == BtStatus_Success);
    M_ASSERT(bt_tree_run_status("betree", "betree2") == BtStatus_Sleep);

    // 行为树2 -----------------------------------------------------------------
    // 测试行为树2，并检查内部数据。
    bt_tree_run_id("betree", 2);
    M_ASSERT(test_betree.act_id == 3);
    M_ASSERT(test_betree.count_node3 == 0);
    M_ASSERT(test_betree.super.bt->node_id_crt_se == 0);
    M_ASSERT(bt_tree_run_status("betree", "betree1") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "betree2") == BtStatus_Running);

    // 在行为节点3的功能函数中，运行count_tick次Evt_Tick
    for (int i = 0; i < count_tick; i ++) {
        EVT_PUB(Evt_Tick);
        M_ASSERT(meow_once() == 0);
        M_ASSERT(test_betree.count_node3 == 1 + i);
    }

    // 订阅Evt_ActEnd事件时进行的单元测试，当前行为节点完成，进行下一个行为节点
    evt_subscribe(obj, Evt_ActEnd);
    EVT_PUB(Evt_ActEnd);
    M_ASSERT(meow_once() == 0);
    M_ASSERT(test_betree.end_node3 == true);
    M_ASSERT(meow_once() == 203);
    M_ASSERT(meow_once() == 202);
    M_ASSERT(bt_tree_run_status("betree", "betree1") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "betree2") == BtStatus_Running);
    M_ASSERT(test_betree.act_id == 4);
    M_ASSERT(test_betree.count_node4 == 0);
    M_ASSERT(test_betree.super.bt->node_id_crt_se == 1);
    M_ASSERT(bt_tree_run_status("betree", "betree1") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "betree2") == BtStatus_Running);

    // 在行为节点2的功能函数中，运行count_tick次Evt_Tick
    for (int i = 0; i < count_tick; i ++) {
        EVT_PUB(Evt_Tick);
        M_ASSERT(meow_once() == 0);
        M_ASSERT(test_betree.count_node4 == 1 + i);
    }
    M_ASSERT(bt_tree_run_status("betree", "betree1") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "betree2") == BtStatus_Running);

    // 发送Evt_ActEnd，行为节点2完成，行为树完成
    EVT_PUB(Evt_ActEnd);
    M_ASSERT(meow_once() == 0);
    M_ASSERT(test_betree.end_node4 == true);
    M_ASSERT(meow_once() == 203);
    M_ASSERT(meow_once() == 202);
    M_ASSERT(bt_tree_run_status("betree", "betree1") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "betree2") == BtStatus_Success);
    M_ASSERT(bt_tree_run_status("betree", "tree_cpmplex") == BtStatus_Sleep);

    // M_ASSERT(ut_betree_execute(obj, evt_new(Evt_ActEnd)) == 5200);

    // 复杂行为树 ---------------------------------------------------------------
    // 运行Root - Sq1 - Sq2 - Ac3 ==========================
    test_betree_clear(&test_betree);
    bt_tree_run("betree", "tree_cpmplex");
    M_ASSERT(test_betree.act_id == 3);
    M_ASSERT(test_betree.count_node3 == 0);
    M_ASSERT(test_betree.super.bt->node_id_crt_se == 0);
    M_ASSERT(bt_tree_run_status("betree", "betree1") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "betree2") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "tree_cpmplex") == BtStatus_Running);

    // 在行为节点3的功能函数中，运行count_tick次Evt_Tick
    for (int i = 0; i < count_tick; i ++) {
        M_ASSERT(ut_betree_execute(obj, evt_new(Evt_Tick)) == 5111);
        M_ASSERT(test_betree.count_node3 == 1 + i);
    }

    // Evt_ActEnd，结束Act3
    test_betree_clear(&test_betree);
    M_ASSERT(ut_betree_execute(obj, evt_new(Evt_ActEnd)) == 5301);

    // 运行Root - Sq1 - Sq2 - Ac4 ==========================
    M_ASSERT(test_betree.act_id == 4);
    M_ASSERT(test_betree.count_node4 == 0);
    M_ASSERT(test_betree.super.bt->node_id_crt_se == 1);
    M_ASSERT(bt_tree_run_status("betree", "betree1") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "betree2") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "tree_cpmplex") == BtStatus_Running);

    // 在行为节点4的功能函数中，运行count_tick次Evt_Tick
    for (int i = 0; i < count_tick; i ++) {
        M_ASSERT(ut_betree_execute(obj, evt_new(Evt_Tick)) == 5111);
        M_ASSERT(test_betree.count_node4 == 1 + i);
    }

    // Evt_ActEnd，结束Act4
    test_betree_clear(&test_betree);
    M_ASSERT(ut_betree_execute(obj, evt_new(Evt_ActEnd)) == 5301);

    // 运行Root - Sq1 - Ac1 ================================
    M_ASSERT(test_betree.act_id == 1);
    M_ASSERT(test_betree.count_node1 == 0);
    M_ASSERT(test_betree.super.bt->node_id_crt_se == 1);
    M_ASSERT(bt_tree_run_status("betree", "betree1") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "betree2") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "tree_cpmplex") == BtStatus_Running);

    // 在行为节点的功能函数中，运行count_tick次Evt_Tick
    for (int i = 0; i < count_tick; i ++) {
        M_ASSERT(ut_betree_execute(obj, evt_new(Evt_Tick)) == 5111);
        M_ASSERT(test_betree.count_node1 == 1 + i);
    }

    // Evt_ActEnd，结束Act1
    test_betree_clear(&test_betree);
    M_ASSERT(ut_betree_execute(obj, evt_new(Evt_ActEnd)) == 5301);

    // 运行Root - Sq1 - Ac2 ================================
    M_ASSERT(test_betree.act_id == 2);
    M_ASSERT(test_betree.count_node1 == 0);
    M_ASSERT(test_betree.super.bt->node_id_crt_se == 2);
    M_ASSERT(bt_tree_run_status("betree", "betree1") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "betree2") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "tree_cpmplex") == BtStatus_Running);

    // 在行为节点的功能函数中，运行count_tick次Evt_Tick
    for (int i = 0; i < count_tick; i ++) {
        M_ASSERT(ut_betree_execute(obj, evt_new(Evt_Tick)) == 5111);
        M_ASSERT(test_betree.count_node2 == 1 + i);
    }

    // Evt_ActEnd，结束Act2，结束Sq1
    test_betree_clear(&test_betree);
    M_ASSERT(ut_betree_execute(obj, evt_new(Evt_ActEnd)) == 5301);

    // 运行Root - Sq1 - Ac1 ================================
    M_ASSERT(test_betree.act_id == 1);
    M_ASSERT(test_betree.count_node1 == 0);
    M_ASSERT(test_betree.super.bt->node_id_crt_se == 0);
    M_ASSERT(bt_tree_run_status("betree", "betree1") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "betree2") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "tree_cpmplex") == BtStatus_Running);

    // 在行为节点的功能函数中，运行count_tick次Evt_Tick
    for (int i = 0; i < count_tick; i ++) {
        M_ASSERT(ut_betree_execute(obj, evt_new(Evt_Tick)) == 5111);
        M_ASSERT(test_betree.count_node1 == 1 + i);
    }

    // Evt_ActEnd，结束Act1，开始运行Pa1
    test_betree_clear(&test_betree);
    M_ASSERT(ut_betree_execute(obj, evt_new(Evt_ActEnd)) == 5302);

    // 运行Root - Sq3 - Pa1 同时运行Ac2和Ac3 ================
    M_ASSERT(test_betree.act_id == 3);
    M_ASSERT(bt_tree_run_status("betree", "betree1") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "betree2") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "tree_cpmplex") == BtStatus_Running);

    // 在行为节点的功能函数中，运行count_tick次Evt_Tick
    for (int i = 0; i < count_tick; i ++) {
        M_ASSERT(ut_betree_execute(obj, evt_new(Evt_Tick)) == 5122);
        M_ASSERT(test_betree.count_node2 == 1 + i);
        M_ASSERT(test_betree.count_node3 == 1 + i);
    }
    M_ASSERT(test_betree.super.bt->stack[0].node->ctrl_type == BtfCtrl_Sequence);
    M_ASSERT(test_betree.super.bt->stack[1].node->ctrl_type == BtfCtrl_Sequence);
    M_ASSERT(test_betree.super.bt->stack[2].node->ctrl_type == BtfCtrl_Parallel);

    // Evt_ActEnd，结束Pa1，开始运行Ac4
    test_betree_clear(&test_betree);
    M_ASSERT(ut_betree_execute(obj, evt_new(Evt_ActEnd)) == 5301);

    // 运行Root - Sq3 - Ac4 ================================
    M_ASSERT(test_betree.act_id == 4);
    M_ASSERT(test_betree.count_node4 == 0);
    M_ASSERT(test_betree.super.bt->node_id_crt_se == 2);
    M_ASSERT(bt_tree_run_status("betree", "betree1") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "betree2") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "tree_cpmplex") == BtStatus_Running);

    // 在行为节点的功能函数中，运行count_tick次Evt_Tick
    for (int i = 0; i < count_tick; i ++) {
        M_ASSERT(ut_betree_execute(obj, evt_new(Evt_Tick)) == 5111);
        M_ASSERT(test_betree.count_node4 == 1 + i);
    }

    // Evt_ActEnd，结束Sq3，开始运行Se1 - Sq5 - Ac3
    test_betree_clear(&test_betree);
    // 设定条件
    set_condition(&test_betree, 1);
    M_ASSERT(ut_betree_execute(obj, evt_new(Evt_ActEnd)) == 5301);

    // 运行Se1 - Sq5 - Ac3 ==================================
    M_ASSERT(test_betree.act_id == 3);
    M_ASSERT(test_betree.count_node3 == 0);
    M_ASSERT(test_betree.super.bt->node_id_crt_se == 0);
    M_ASSERT(bt_tree_run_status("betree", "betree1") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "betree2") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "tree_cpmplex") == BtStatus_Running);

    // 在行为节点的功能函数中，运行count_tick次Evt_Tick
    for (int i = 0; i < count_tick; i ++) {
        M_ASSERT(ut_betree_execute(obj, evt_new(Evt_Tick)) == 5111);
        M_ASSERT(test_betree.count_node3 == 1 + i);
    }

    // Evt_ActEnd，结束Sq5 - Ac3
    test_betree_clear(&test_betree);
    M_ASSERT(ut_betree_execute(obj, evt_new(Evt_ActEnd)) == 5301);
    M_ASSERT(bt_tree_run_status("betree", "betree1") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "betree2") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "tree_cpmplex") == BtStatus_Running);

    // 运行Se1 - Sq5 - Ac4 ==================================
    M_ASSERT(test_betree.act_id == 4);
    M_ASSERT(test_betree.count_node4 == 0);
    M_ASSERT(test_betree.super.bt->node_id_crt_se == 1);
    M_ASSERT(bt_tree_run_status("betree", "betree1") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "betree2") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "tree_cpmplex") == BtStatus_Running);

    // 在行为节点的功能函数中，运行count_tick次Evt_Tick
    for (int i = 0; i < count_tick; i ++) {
        M_ASSERT(ut_betree_execute(obj, evt_new(Evt_Tick)) == 5111);
        M_ASSERT(test_betree.count_node4 == 1 + i);
    }

    // Evt_ActEnd，结束Sq5，结束Se1，结束整个行为树
    test_betree_clear(&test_betree);
    M_ASSERT(ut_betree_execute(obj, evt_new(Evt_ActEnd)) == 5200);
    M_ASSERT(bt_tree_run_status("betree", "betree1") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "betree2") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "tree_cpmplex") == BtStatus_Success);

    // 复杂行为树
    /*
        root_____Sq1_______Sq2________Ac3
        |        |           |________Ac4
        |        |_____Ac1
        |        |_____Ac2
        |
        |_______Sq3____Ac1
        |        |
        |        |____Pa1______Ac2
        |        |     |_______Ac3
        |        |
        |        |_____Ac4
        |
        |_______Se1__c1____Sq5_____Ac3
                |           |______Ac4
                |
                |___c2____Sq6_____Pa2_____Ac1
                            |      |______Ac2
                            |
                            |______Ac3
                            |
                            |______Se2___c3____Ac4
                                    |____c4____Ac1
    */

    // 复杂行为树 ---------------------------------------------------------------
    // 此次执行选择节点的其他分支

    // 运行Root - Sq1 - Sq2 - Ac3 ==========================
    test_betree_clear(&test_betree);
    bt_tree_run("betree", "tree_cpmplex");
    M_ASSERT(test_betree.act_id == 3);
    M_ASSERT(test_betree.count_node3 == 0);
    M_ASSERT(test_betree.super.bt->node_id_crt_se == 0);
    M_ASSERT(bt_tree_run_status("betree", "betree1") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "betree2") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "tree_cpmplex") == BtStatus_Running);

    // 在行为节点3的功能函数中，运行count_tick次Evt_Tick
    for (int i = 0; i < count_tick; i ++) {
        M_ASSERT(ut_betree_execute(obj, evt_new(Evt_Tick)) == 5111);
        M_ASSERT(test_betree.count_node3 == 1 + i);
    }

    // Evt_ActEnd，结束Act3
    test_betree_clear(&test_betree);
    M_ASSERT(ut_betree_execute(obj, evt_new(Evt_ActEnd)) == 5301);

    // 运行Root - Sq1 - Sq2 - Ac4 ==========================
    M_ASSERT(test_betree.act_id == 4);
    M_ASSERT(test_betree.count_node4 == 0);
    M_ASSERT(test_betree.super.bt->node_id_crt_se == 1);
    M_ASSERT(bt_tree_run_status("betree", "betree1") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "betree2") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "tree_cpmplex") == BtStatus_Running);

    // 在行为节点4的功能函数中，运行count_tick次Evt_Tick
    for (int i = 0; i < count_tick; i ++) {
        M_ASSERT(ut_betree_execute(obj, evt_new(Evt_Tick)) == 5111);
        M_ASSERT(test_betree.count_node4 == 1 + i);
    }

    // Evt_ActEnd，结束Act4
    test_betree_clear(&test_betree);
    M_ASSERT(ut_betree_execute(obj, evt_new(Evt_ActEnd)) == 5301);

    // 运行Root - Sq1 - Ac1 ================================
    M_ASSERT(test_betree.act_id == 1);
    M_ASSERT(test_betree.count_node1 == 0);
    M_ASSERT(test_betree.super.bt->node_id_crt_se == 1);
    M_ASSERT(bt_tree_run_status("betree", "betree1") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "betree2") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "tree_cpmplex") == BtStatus_Running);

    // 在行为节点的功能函数中，运行count_tick次Evt_Tick
    for (int i = 0; i < count_tick; i ++) {
        M_ASSERT(ut_betree_execute(obj, evt_new(Evt_Tick)) == 5111);
        M_ASSERT(test_betree.count_node1 == 1 + i);
    }

    // Evt_ActEnd，结束Act1
    test_betree_clear(&test_betree);
    M_ASSERT(ut_betree_execute(obj, evt_new(Evt_ActEnd)) == 5301);

    // 运行Root - Sq1 - Ac2 ================================
    M_ASSERT(test_betree.act_id == 2);
    M_ASSERT(test_betree.count_node1 == 0);
    M_ASSERT(test_betree.super.bt->node_id_crt_se == 2);
    M_ASSERT(bt_tree_run_status("betree", "betree1") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "betree2") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "tree_cpmplex") == BtStatus_Running);

    // 在行为节点的功能函数中，运行count_tick次Evt_Tick
    for (int i = 0; i < count_tick; i ++) {
        M_ASSERT(ut_betree_execute(obj, evt_new(Evt_Tick)) == 5111);
        M_ASSERT(test_betree.count_node2 == 1 + i);
    }

    // Evt_ActEnd，结束Act2，结束Sq1
    test_betree_clear(&test_betree);
    M_ASSERT(ut_betree_execute(obj, evt_new(Evt_ActEnd)) == 5301);

    // 运行Root - Sq1 - Ac1 ================================
    M_ASSERT(test_betree.act_id == 1);
    M_ASSERT(test_betree.count_node1 == 0);
    M_ASSERT(test_betree.super.bt->node_id_crt_se == 0);
    M_ASSERT(bt_tree_run_status("betree", "betree1") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "betree2") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "tree_cpmplex") == BtStatus_Running);

    // 在行为节点的功能函数中，运行count_tick次Evt_Tick
    for (int i = 0; i < count_tick; i ++) {
        M_ASSERT(ut_betree_execute(obj, evt_new(Evt_Tick)) == 5111);
        M_ASSERT(test_betree.count_node1 == 1 + i);
    }

    // Evt_ActEnd，结束Act1，开始运行Pa1
    test_betree_clear(&test_betree);
    M_ASSERT(ut_betree_execute(obj, evt_new(Evt_ActEnd)) == 5302);

    // 运行Root - Sq3 - Pa1 同时运行Ac2和Ac3 ================
    M_ASSERT(test_betree.act_id == 3);
    M_ASSERT(bt_tree_run_status("betree", "betree1") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "betree2") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "tree_cpmplex") == BtStatus_Running);

    // 在行为节点的功能函数中，运行count_tick次Evt_Tick
    for (int i = 0; i < count_tick; i ++) {
        M_ASSERT(ut_betree_execute(obj, evt_new(Evt_Tick)) == 5122);
        M_ASSERT(test_betree.count_node2 == 1 + i);
        M_ASSERT(test_betree.count_node3 == 1 + i);
    }
    M_ASSERT(test_betree.super.bt->stack[0].node->ctrl_type == BtfCtrl_Sequence);
    M_ASSERT(test_betree.super.bt->stack[1].node->ctrl_type == BtfCtrl_Sequence);
    M_ASSERT(test_betree.super.bt->stack[2].node->ctrl_type == BtfCtrl_Parallel);

    // Evt_ActEnd，结束Pa1，开始运行Ac4
    test_betree_clear(&test_betree);
    M_ASSERT(ut_betree_execute(obj, evt_new(Evt_ActEnd)) == 5301);

    // 运行Root - Sq3 - Ac4 ================================
    M_ASSERT(test_betree.act_id == 4);
    M_ASSERT(test_betree.count_node4 == 0);
    M_ASSERT(test_betree.super.bt->node_id_crt_se == 2);
    M_ASSERT(bt_tree_run_status("betree", "betree1") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "betree2") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "tree_cpmplex") == BtStatus_Running);

    // 在行为节点的功能函数中，运行count_tick次Evt_Tick
    for (int i = 0; i < count_tick; i ++) {
        M_ASSERT(ut_betree_execute(obj, evt_new(Evt_Tick)) == 5111);
        M_ASSERT(test_betree.count_node4 == 1 + i);
    }

    // Evt_ActEnd，结束Sq3，开始运行Se1 - Sq5 - Ac3
    test_betree_clear(&test_betree);
    // 设定条件
    set_condition(&test_betree, 2);
    M_ASSERT(ut_betree_execute(obj, evt_new(Evt_ActEnd)) == 5302);

    // 运行Se1 - Sq6 - Pa2   ==================================
    // printf("test_betree.act_id: %d.", test_betree.act_id);
    M_ASSERT(test_betree.act_id == 2);
    M_ASSERT(test_betree.count_node1 == 0);
    M_ASSERT(test_betree.count_node2 == 0);
    M_ASSERT(bt_tree_run_status("betree", "betree1") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "betree2") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "tree_cpmplex") == BtStatus_Running);

    // 在行为节点的功能函数中，运行count_tick次Evt_Tick
    for (int i = 0; i < count_tick; i ++) {
        M_ASSERT(ut_betree_execute(obj, evt_new(Evt_Tick)) == 5122);
        M_ASSERT(test_betree.count_node1 == 1 + i);
        M_ASSERT(test_betree.count_node2 == 1 + i);
    }

    // Evt_ActEnd，结束Se1 - Sq6 - Pa2
    test_betree_clear(&test_betree);
    M_ASSERT(ut_betree_execute(obj, evt_new(Evt_ActEnd)) == 5301);
    M_ASSERT(bt_tree_run_status("betree", "betree1") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "betree2") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "tree_cpmplex") == BtStatus_Running);

    // 运行Se1 - Sq6 - Ac3   ==================================
    M_ASSERT(test_betree.act_id == 3);
    M_ASSERT(test_betree.count_node3 == 0);
    M_ASSERT(test_betree.super.bt->node_id_crt_se == 1);
    M_ASSERT(bt_tree_run_status("betree", "betree1") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "betree2") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "tree_cpmplex") == BtStatus_Running);

    // 在行为节点的功能函数中，运行count_tick次Evt_Tick
    for (int i = 0; i < count_tick; i ++) {
        M_ASSERT(ut_betree_execute(obj, evt_new(Evt_Tick)) == 5111);
        M_ASSERT(test_betree.count_node3 == 1 + i);
    }

    // Evt_ActEnd，结束Se1 - Sq6 - Ac3
    test_betree_clear(&test_betree);
    // 设定条件
    set_condition(&test_betree, 3);
    M_ASSERT(ut_betree_execute(obj, evt_new(Evt_ActEnd)) == 5303);
    M_ASSERT(bt_tree_run_status("betree", "betree1") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "betree2") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "tree_cpmplex") == BtStatus_Running);

    // 运行Se1 - Sq6 - Se2 - Ac4 ==================================
    M_ASSERT(test_betree.act_id == 4);
    M_ASSERT(test_betree.count_node4 == 0);
    M_ASSERT(bt_tree_run_status("betree", "betree1") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "betree2") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "tree_cpmplex") == BtStatus_Running);

    // 在行为节点的功能函数中，运行count_tick次Evt_Tick
    for (int i = 0; i < count_tick; i ++) {
        M_ASSERT(ut_betree_execute(obj, evt_new(Evt_Tick)) == 5132);
        M_ASSERT(test_betree.count_node4 == 1 + i);
    }

    // Evt_ActEnd，结束Se1 - Sq6 - Se2 - Ac4，结束整个行为树。
    M_ASSERT(ut_betree_execute(obj, evt_new(Evt_ActEnd)) == 5200);
    M_ASSERT(bt_tree_run_status("betree", "betree1") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "betree2") == BtStatus_Sleep);
    M_ASSERT(bt_tree_run_status("betree", "tree_cpmplex") == BtStatus_Success);

    // 检查运行不存在的行为树ID，通过。
    // bt_tree_run_id("betree", 4);
}

// static function -------------------------------------------------------------
    // 复杂行为树
    /*
        root_____Sq1_______Sq2________Ac3
        |        |           |________Ac4
        |        |_____Ac1
        |        |_____Ac2
        |
        |_______Sq3____Ac1
        |        |
        |        |____Pa1______Ac2
        |        |     |_______Ac3
        |        |
        |        |_____Ac4
        |
        |_______Se1__c1____Sq5_____Ac3
                |           |______Ac4
                |
                |___c2____Sq6_____Pa2_____Ac1
                            |      |______Ac2
                            |
                            |______Ac3
                            |
                            |______Se2___c3____Ac4
                                    |____c4____Ac1
    */
static void test_betree_init(test_betree_t * const test_betree,
                             const char * name, int pri, int fifo_depth)
{
    // 新建对象
    obj = (m_obj_t *)&test_betree->super;
    obj_init(obj, name, ObjType_BeTree, pri, fifo_depth);
    test_betree->count_node1 = 0;
    test_betree->count_node2 = 0;

    // 新建4个行为节点
    int para[MEOW_EVT_PARAS_NUM];
    btf_node_t * act1 = bt_node_act_create(ACT_CAST(act_node_1), para);
    btf_node_t * act2 = bt_node_act_create(ACT_CAST(act_node_2), para);
    btf_node_t * act3 = bt_node_act_create(ACT_CAST(act_node_3), para);
    btf_node_t * act4 = bt_node_act_create(ACT_CAST(act_node_4), para);

    // 新建行为树1
    btf_node_t * ctrl1 = bt_node_ctrl_create(BtfCtrl_Sequence);
    M_ASSERT(ctrl1->ctrl_type == BtfCtrl_Sequence);

    bt_ctrl_add_node(ctrl1, act1);
    bt_ctrl_add_node(ctrl1, act2);

    betree_t * tree1 = bt_tree_create("betree1", 1, ctrl1);
    bt_add_betree(obj, tree1);

    M_ASSERT(tree1->depth == 1);
    M_ASSERT(obj->bt->betree_list == tree1);
    M_ASSERT(obj->bt->betree_list->next == (betree_t *)0);

    // 新建行为树2
    btf_node_t * ctrl2 = bt_node_ctrl_create(BtfCtrl_Sequence);

    bt_ctrl_add_node(ctrl2, act3);
    bt_ctrl_add_node(ctrl2, act4);

    betree_t * tree2 = bt_tree_create("betree2", 2, ctrl2);
    bt_add_betree(obj, tree2);

    M_ASSERT(tree2->depth == 1);
    M_ASSERT(obj->bt->betree_list->next == tree2);
    M_ASSERT(obj->bt->betree_list->next->next == (betree_t *)0);

    // 新建复杂行为树，包含各种情况，用于测试bt_execute函数
    // sq2
    btf_node_t * sq2 = bt_node_ctrl_create(BtfCtrl_Sequence);
    bt_ctrl_add_node(sq2, act3);
    bt_ctrl_add_node(sq2, act4);
    M_ASSERT(sq2->ctrl_type == BtfCtrl_Sequence);

    // sq1
    btf_node_t * sq1 = bt_node_ctrl_create(BtfCtrl_Sequence);
    bt_ctrl_add_node(sq1, sq2);
    bt_ctrl_add_node(sq1, act1);
    bt_ctrl_add_node(sq1, act2);
    M_ASSERT(sq1->ctrl_type == BtfCtrl_Sequence);

    // pa1
    btf_node_t * pa1 = bt_node_ctrl_create(BtfCtrl_Parallel);
    bt_ctrl_add_node(pa1, act2);
    bt_ctrl_add_node(pa1, act3);
    M_ASSERT(pa1->ctrl_type == BtfCtrl_Parallel);

    // sq3
    btf_node_t * sq3 = bt_node_ctrl_create(BtfCtrl_Sequence);
    bt_ctrl_add_node(sq3, act1);
    bt_ctrl_add_node(sq3, pa1);
    bt_ctrl_add_node(sq3, act4);
    M_ASSERT(sq3->ctrl_type == BtfCtrl_Sequence);

    // sq5
    btf_node_t * sq5 = bt_node_ctrl_create(BtfCtrl_Sequence);
    bt_ctrl_add_node(sq5, act3);
    bt_ctrl_add_node(sq5, act4);
    M_ASSERT(sq5->ctrl_type == BtfCtrl_Sequence);

    // pa2
    btf_node_t * pa2 = bt_node_ctrl_create(BtfCtrl_Parallel);
    bt_ctrl_add_node(pa2, act1);
    bt_ctrl_add_node(pa2, act2);
    M_ASSERT(pa2->ctrl_type == BtfCtrl_Parallel);

    // se2
    btf_node_t * se2 = bt_node_ctrl_create(BtfCtrl_Select);
    bt_ctrl_add_node_condition(se2, act4, CONDITION_CAST(condition_3));
    bt_ctrl_add_node_condition(se2, act1, CONDITION_CAST(condition_4));
    M_ASSERT(se2->ctrl_type == BtfCtrl_Select);

    // sq6
    btf_node_t * sq6 = bt_node_ctrl_create(BtfCtrl_Sequence);
    bt_ctrl_add_node(sq6, pa2);
    bt_ctrl_add_node(sq6, act3);
    bt_ctrl_add_node(sq6, se2);
    M_ASSERT(sq6->ctrl_type == BtfCtrl_Sequence);

    // se1
    btf_node_t * se1 = bt_node_ctrl_create(BtfCtrl_Select);
    bt_ctrl_add_node_condition(se1, sq5, CONDITION_CAST(condition_1));
    bt_ctrl_add_node_condition(se1, sq6, CONDITION_CAST(condition_2));
    M_ASSERT(se1->ctrl_type == BtfCtrl_Select);

    // root
    btf_node_t * root = bt_node_ctrl_create(BtfCtrl_Sequence);
    bt_ctrl_add_node(root, sq1);
    bt_ctrl_add_node(root, sq3);
    bt_ctrl_add_node(root, se1);
    M_ASSERT(root->ctrl_type == BtfCtrl_Sequence);

    // betree
    betree_t * tree_complex = bt_tree_create("tree_cpmplex", 3, root);
    bt_add_betree(obj, tree_complex);
    M_ASSERT(obj->bt->betree_list == tree1);
    M_ASSERT(obj->bt->betree_list->next == tree2);
    M_ASSERT(obj->bt->betree_list->next->next == tree_complex);
    M_ASSERT(obj->bt->betree_list->next->next->next == (betree_t *)0);
    M_ASSERT(tree_complex->depth == 4);
}

// act function ----------------------------------------------------------------
static m_ret_t act_node_1(test_betree_t * const me, m_evt_t const * const e, int para[])
{
    switch (e->sig) {
        case Evt_Enter:
            me->act_id = 1;
            me->end_node1 = false;
            return M_Ret_Running;

        case Evt_Exit:
            return M_Ret_Success;

        case Evt_Tick:
            me->count_node1 ++;
            return M_Ret_Running;

        case Evt_ActEnd:
            me->end_node1 = true;
            return M_Ret_Success;

        default:
            return M_Ret_Null;
    }
}

static m_ret_t act_node_2(test_betree_t * const me, m_evt_t const * const e, int para[])
{
    switch (e->sig) {
        case Evt_Enter:
            me->act_id = 2;
            me->end_node2 = false;
            return M_Ret_Running;

        case Evt_Tick:
            me->count_node2 ++;
            return M_Ret_Running;

        case Evt_ActEnd:
            me->end_node2 = true;
            return M_Ret_Success;

        default:
            return M_Ret_Null;
    }
}

static m_ret_t act_node_3(test_betree_t * const me, m_evt_t const * const e, int para[])
{
    switch (e->sig) {
        case Evt_Enter:
            me->act_id = 3;
            me->end_node3 = false;
            return M_Ret_Running;

        case Evt_Exit:
            return M_Ret_Success;

        case Evt_Tick:
            me->count_node3 ++;
            return M_Ret_Running;

        case Evt_ActEnd:
            me->end_node3 = true;
            return M_Ret_Success;

        default:
            return M_Ret_Null;
    }
}

static m_ret_t act_node_4(test_betree_t * const me, m_evt_t const * const e, int para[])
{
    switch (e->sig) {
        case Evt_Enter:
            me->act_id = 4;
            me->end_node4 = false;
            return M_Ret_Running;

        case Evt_Tick:
            me->count_node4 ++;
            return M_Ret_Running;

        case Evt_ActEnd:
            me->end_node4 = true;
            return M_Ret_Success;

        default:
            return M_Ret_Null;
    }
}

static bool condition_1(test_betree_t * const me)
{
    if (me->condition_value == 1) 
        return true;
    else
        return false;
}

static bool condition_2(test_betree_t * const me)
{
    if (me->condition_value == 2)
        return true;
    else
        return false;
}

static bool condition_3(test_betree_t * const me)
{
    if (me->condition_value == 3)
        return true;
    else
        return false;
}

static bool condition_4(test_betree_t * const me)
{
    if (me->condition_value == 4)
        return true;
    else
        return false;
}

static void set_condition(test_betree_t * const me, int value)
{
    me->condition_value = value;
}

static void test_betree_clear(test_betree_t * const me)
{
    me->act_id = 0;
    me->condition_value = 0;
    me->count_node1 = 0;
    me->count_node2 = 0;
    me->count_node3 = 0;
    me->count_node4 = 0;
    me->end_node1 = false;
    me->end_node2 = false;
    me->end_node3 = false;
    me->end_node4 = false;
}

static m_evt_t * evt_new(int evt_id)
{
    static m_evt_t e;
    e.sig = evt_id;
    e.mode = EvtMode_Ps;
    
    return &e;
}
