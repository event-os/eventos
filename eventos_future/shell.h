
#ifndef __MOD_SHELL_H__
#define __MOD_SHELL_H__

// include ---------------------------------------------------------------------
#include "stdbool.h"
#include "stdint.h"

// key id ----------------------------------------------------------------------
enum {
    Key_F1 = 0, Key_F2, Key_F3, Key_F4,
    Key_Up, Key_Down, Key_Right, Key_Left,
    Key_Home, Key_Insert, Key_Delect, Key_End, Key_PageUp, Key_PageDown,
    Key_F9, Key_F10, Key_F11, Key_F12,
    Key_F5, Key_F6, Key_F7, Key_F8,
    Key_Max
};

// 各个模式的用途
// 响应模式 - 用于指令或者数据的一次性显示
// 刷新模式 - 用于数据的刷新显示
// 增量模式 - 用于Log等不断增加的数据的显示
// 滚动模式 - 用于某文件的显示
enum {
    ShellMode_Act = 1,                      // 响应模式
    ShellMode_Refresh = 2,                  // 刷新模式
    ShellMode_Log = 4,                      // 日志模式
    ShellMode_Scroll = 8,                   // 滚动模式
    ShellMode_FileRecv = 0x10               // 文件接收模式
};

/* config ------------------------------------------------------------------- */
#define SHELL_BUFF_RCV_SIZE                 90
#define SHELL_BUFF_SEND_SIZE                1024
#define SHELL_BUFF_CMD_SIZE                 80
#define SHELL_BUFF_HISTORY_SIZE             4096
#define SHELL_CMD_TABLE_SIZE                32

#define SHELL_REFRESH_TAB_SIZE              32    // 每个刷新表所能容纳的Key个数
#define SHELL_REFRESH_KEY_WIDTH             32    // 刷新显示时，key占的字符宽度

// data struct -----------------------------------------------------------------
typedef void (* shell_hook_t)(char * mode, char * func, int num, char ** para);
typedef void (* shell_fkey_hook_t)(uint8_t key_id);

// API -------------------------------------------------------------------------
// 默认进入Cmd模式
void shell_init(void);
// Shell启动
void shell_start(void);
// Shell模式
void shell_mode(int mode);
// 以刷新形式显示某个数据
void shell_db_refresh(const char * key);
// 注册刷新模式下的特殊功能键的回调函数
void shell_set_fkey_hook(shell_fkey_hook_t fkey_hook);
void shell_set_fkey_hook_default(shell_fkey_hook_t fkey_hook);
// 注册指令
void shell_cmd_reg(const char * cmd, shell_hook_t hook, const char * help);
// 返回字符串长度
int shell_print(const char * format, ...);

// port ------------------------------------------------------------------------
void * shell_port_malloc(int size);
void shell_port_io_init(void);
void shell_port_send(void * data, int size);
void shell_port_set_recv_evt(int evt_id);
int shell_port_recv(void * data, int size);
void shell_port_default_func_reg(void);

#endif
