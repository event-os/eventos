
#ifndef __ESH_H__
#define __ESH_H__

// include ---------------------------------------------------------------------
#include "stdbool.h"
#include "stdint.h"

// 0 - EventOS Shell Client
// 1 - J-Link RTT Client
// 2 - X-Shell
// 3 - MobaXterm
#define EOS_SHELL_CLIENT                   1

// key id ----------------------------------------------------------------------
enum
{
    Esh_Null = 128,
    // 特殊功能键
    Esh_F1, Esh_F2, Esh_F3, Esh_F4,
    Esh_F5, Esh_F6, Esh_F7, Esh_F8,
    Esh_F9, Esh_F10, 
#if (EOS_SHELL_CLIENT != 1)
    Esh_F11,
#endif
    Esh_F12,
    // 上下左右
    Esh_Up, Esh_Down, Esh_Right, Esh_Left,
    // 小键盘键
    Esh_Home, Esh_Insert, Esh_Delect, Esh_End, Esh_PageUp, Esh_PageDown,

    Esh_Ctrl_Start,
#if (EOS_SHELL_CLIENT == 1)
    // 组合键
    Esh_Ctrl_B = Esh_Ctrl_Start, Esh_Ctrl_D, Esh_Ctrl_E, Esh_Ctrl_G,
    Esh_Ctrl_H, Esh_Ctrl_I, Esh_Ctrl_J, Esh_Ctrl_K, Esh_Ctrl_L,
    Esh_Ctrl_O, Esh_Ctrl_P, Esh_Ctrl_Q, Esh_Ctrl_R, Esh_Ctrl_S, Esh_Ctrl_T,
    Esh_Ctrl_U, Esh_Ctrl_W, Esh_Ctrl_X, Esh_Ctrl_Y, Esh_Ctrl_Z,
#endif

#if (EOS_SHELL_CLIENT == 2)
    // 组合键
    Esh_Ctrl_A = Esh_Ctrl_Start,
    Esh_Ctrl_B, Esh_Ctrl_C, Esh_Ctrl_D, Esh_Ctrl_E, Esh_Ctrl_F, Esh_Ctrl_G,
    Esh_Ctrl_I, Esh_Ctrl_K, Esh_Ctrl_L, Esh_Ctrl_N,
    Esh_Ctrl_O, Esh_Ctrl_P, Esh_Ctrl_Q, Esh_Ctrl_R, Esh_Ctrl_S, Esh_Ctrl_T,
    Esh_Ctrl_U, Esh_Ctrl_V, Esh_Ctrl_W, Esh_Ctrl_X, Esh_Ctrl_Y, Esh_Ctrl_Z,
#endif

    Esh_Max
};

// 各个模式的用途
// 响应模式 - 用于指令或者数据的一次性显示
// 日志模式 - 用于Log等不断增加的数据的显示
// 接收模式 - 用于Y-Modem协议接收文件
enum
{
    EshMode_Act = 0,                        // 响应模式
    EshMode_Log,                            // 日志模式
    EshMode_FileRecv                        // 文件接收模式
};

/* config ------------------------------------------------------------------- */
#define ESH_BUFF_RECV_SIZE                  90
#define ESH_BUFF_SEND_SIZE                  2048
#define ESH_BUFF_CMD_SIZE                   80
#define ESH_REG_TABLE_SIZE                  32

// data struct -----------------------------------------------------------------
typedef void (* esh_hook_t)(char * mode, char * func, int num, char ** para);
typedef void (* esh_fkey_hook_t)(uint8_t key_id);

// API -------------------------------------------------------------------------
// 默认进入Log模式
void esh_init(uint32_t evt_stop);
// Shell启动
void esh_start(void);
// Msh Ready
bool esh_ready(void);
// Shell模式
void esh_mode(uint32_t mode);
// 注册刷新模式下的特殊功能键的回调函数
void esh_set_fkey_hook(esh_fkey_hook_t fkey_hook);
void esh_set_fkey_hook_default(esh_fkey_hook_t fkey_hook);
// 注册指令
void esh_cmd_reg(const char * cmd, uint8_t shortcut, esh_hook_t hook, const char * help);
// 打印LOG
void esh_log(const char * log);
// msh的轮询
void esh_poll(uint64_t time_ms);

// port ------------------------------------------------------------------------
void esh_port_init(void);
void esh_port_send(void * data, uint32_t size);
uint8_t esh_port_recv(void * data, uint8_t size);

#endif
