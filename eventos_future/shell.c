
// include ---------------------------------------------------------------------
#include "shell.h"
#include "evt_def.h"
#include "obj_def.h"
#include "meow.h"
#include "mdb/mdb.h"
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "mlog/mlog.h"
#include "mlog/mlog.h"

M_TAG("Mod-Shell")

// todo ------------------------------------------------------------------------
// TODO 对于过程的const char *类型的数据，分多次发送，避免对send_buffer造成过大的
//      压力。
// TODO 添加UP键和DOWN键获取历史指令功能。
// TODO 后续修改为在文件中存储历史指令，或者在Flash中存储历史指令。

// key value -------------------------------------------------------------------
/*
    201 Key_F1 = 201,               // 1B 4F 50
    202 Key_F2,                     // 1B 4F 51
    203 Key_F3,                     // 1B 4F 52
    204 Key_F4,                     // 1B 4F 53
    205 Key_Up,                     // 1B 5B 41
    206 Key_Down,                   // 1B 5B 42
    207 Key_Right,                  // 1B 5B 43
    208 Key_Left,                   // 1B 5B 44

    209 Key_Home,                   // 1B 5B 31 7E
    210 Key_Insert,                 // 1B 5B 32 7E
    211 Key_Delect,                 // 1B 5B 33 7E
    212 Key_End,                    // 1B 5B 34 7E
    213 Key_PageUp,                 // 1B 5B 35 7E
    214 Key_PageDown,               // 1B 5B 36 7E

    215 Key_F9,                     // 1B 5B 32 30 7E
    216 Key_F10,                    // 1B 5B 32 31 7E
    218 Key_F11 = Key_F10 + 2,      // 1B 5B 32 33 7E
    219 Key_F12,                    // 1B 5B 32 34 7E

    220 Key_F5,                     // 1B 5B 31 35 7E
    222 Key_F6 = Key_F5 + 2,        // 1B 5B 31 37 7E
    223 Key_F7,                     // 1B 5B 31 38 7E
    224 Key_F8,                     // 1B 5B 31 39 7E
*/

// macro -----------------------------------------------------------------------
#define PT_CMD_MAX                              16
#define SH_ESC_OUT_MAX                          16
#define SHELL_BUFF_SIZE                         16

#define FKEY_TOTAL_NUM                          16

// 特殊功能键数据 ---------------------------------------------------------------
// 每个特殊功能键，绑定一个指令
typedef struct shell_fkey_data_tag {
    const char * name;
    uint8_t id;
} shell_fkey_data_t;

enum {
    Key_BackSpace = 0x08,
    Key_Space = 0x20,
    Key_Enter = 0x0D,
    Key_Esc = 0x1B,
    Key_NewLine = 0x0A,
};

static const shell_fkey_data_t fkey_data[] = {
    { "Key_F1", 201 }, { "Key_F2", 202 }, { "Key_F3", 203 }, { "Key_F4", 204 },
    { "Key_Up", 205 }, { "Key_Down", 206 }, { "Key_Right", 207 }, { "Key_Left", 208 },
    { "Key_Home", 209 }, { "Key_Insert", 210 }, { "Key_Delect", 211 },
    { "Key_End", 212 }, { "Key_PageUp", 213 }, { "Key_PageDown", 214 },
    { "Key_F9", 215 }, { "Key_F10", 216 }, { "Key_F11", 218 }, { "Key_F12", 219 },
    { "Key_F5", 210 }, { "Key_F6", 212 }, { "Key_F7", 213 }, { "Key_F8", 214 }
};

// data struct -----------------------------------------------------------------
// 指令数据
typedef struct sh_cmd_data_tag {
    const char * name;                              // 指令名称
    shell_hook_t hook;                              // 回调函数
    const char * help;
} sh_cmd_data_t;

static void shell_hook_null(char * mode, char * func, int num, char ** para);
static void shell_fkey_hook_defualt(uint8_t key_id);
static void shell_fkey_hook_null(uint8_t key_id);

// refresh data ----------------------------------------------------------------
typedef struct sh_refresh_data_tag {
    struct sh_refresh_data_tag * next;
    const char * keys[SHELL_REFRESH_TAB_SIZE];
    int value;
} sh_refresh_data_t;

// class -----------------------------------------------------------------------
typedef struct shell_tag {
    m_obj_t super;
    int mode;

    // 刷新表
    sh_refresh_data_t * refresh_list;

    // 指令Buffer
    char cmd_buffer[SHELL_BUFF_CMD_SIZE + 1];
    // 当前指令备份，用于翻到历史指令又翻回来，恢复当前已输入指令的情况。
    char cmd_buffer_bkp[SHELL_BUFF_CMD_SIZE + 1];
    uint16_t count_cmd_char;
    char cmd_history[SHELL_BUFF_HISTORY_SIZE];
    int head_history;                               // 最新历史指令的头
    int head_history_temp;                          // 在Up Down翻动时，历史指令的头
    bool cmd_histroy_empty;
    // 接收与发送Buffer
    uint8_t buff_rcv[SHELL_BUFF_RCV_SIZE];
    int count_recv;
    uint8_t buff_send[SHELL_BUFF_SEND_SIZE];
    int count_send;

    sh_cmd_data_t cmd_table[SHELL_CMD_TABLE_SIZE];  // 指令表
    int count_cmd;
    const char * name_module;                       // Assert
    int line_num;

    int time_count_db;                              // 数据显示刷新的计时
    int char_count_refresh;                         // 数据刷新的个数

    shell_fkey_hook_t fkey_hook;
    shell_fkey_hook_t fkey_hook_default;
    bool en_log;

    uint8_t buff_esc[128];
    uint16_t count_esc;
} shell_t;

shell_t shell;

// hooks -----------------------------------------------------------------------
static void hook_help(char * mode, char * func, int num, char ** para);

// default shell cmd table -----------------------------------------------------
#define HELP_INFORMATION "MShell, version 2.0-alpha.\n\r\
These shell commands are defined internally. Type 'help' to see this list.\n\r\
Type 'command --help' to find out more information about the function 'name'.\n\r\
\n\r\
base        manually move the robot and check the data.\n\r\
mdb         read and display k-v datas in database.\n\r\
log(*)      log function. setting and display current log information or former\n\r\
            log in the file system.\n\r\
file(*)     read file from flash-rom and display its data in terminal.\n\r\
ls(*)       list all folders and files in the current path.\n\r\
zcp(*)      copy one file from host to the mcu file system.\n\r\
rm(*)       remove the files.\n\r\
\n\r\
(*) The commands are not supported until now.\n\r\
"
static const sh_cmd_data_t sh_cmdtable_def[] = {
    { "help", hook_help, HELP_INFORMATION},
};

// private data ----------------------------------------------------------------
static char * data_newline = "\n\r>> ";

// private function ------------------------------------------------------------
static bool sh_is_cmd_char(uint8_t cmd_char);
static void sh_key_value_func(uint8_t * p_data, uint8_t count);
static void sh_clear_buff_cmd(void);
static bool sh_is_num_and_char(uint8_t cmd_char);
static uint8_t shell_get_fkey_id(int key_id);

// private state - sm_shell ----------------------------------------------------
static m_ret_t state_init(shell_t * const me, m_evt_t const * const e);
static m_ret_t state_unready(shell_t * const me, m_evt_t const * const e);
static m_ret_t state_work(shell_t * const me, m_evt_t const * const e);
    static m_ret_t state_act(shell_t * const me, m_evt_t const * const e);
    static m_ret_t state_refresh(shell_t * const me, m_evt_t const * const e);
    static m_ret_t state_log(shell_t * const me, m_evt_t const * const e);
    static m_ret_t state_scroll(shell_t * const me, m_evt_t const * const e);
    static m_ret_t state_filerecv(shell_t * const me, m_evt_t const * const e);

// private function ------------------------------------------------------------
static void shell_cmd_parser(shell_t * const me);
static void shell_publish_key(uint8_t key_id);
static void shell_cmd_history(shell_t * const me, bool history_up);

// API -------------------------------------------------------------------------
void shell_init(void)
{
    shell.mode = ShellMode_Act;
    shell.count_cmd = 0;

    // 对各个Buffer进行初始化。
    for (int i = 0; i < SHELL_BUFF_CMD_SIZE + 1; i ++) 
        shell.cmd_buffer[i] = 0;
    for (int i = 0; i < SHELL_BUFF_RCV_SIZE; i ++)
        shell.buff_rcv[i] = 0;
    for (int i = 0; i < SHELL_BUFF_SEND_SIZE; i ++)
        shell.buff_send[i] = 0;
    shell.count_cmd_char = 0;
    shell.count_recv = 0;
    shell.count_send = 0;
    shell.cmd_histroy_empty = true;
    shell.head_history_temp = INT32_MAX;

    // 刷新表
    shell.refresh_list = shell_port_malloc(sizeof(sh_refresh_data_t));
    M_ASSERT(shell.refresh_list != (void *)0);
    shell.refresh_list->next = 0;
    for (int i = 0; i < SHELL_REFRESH_TAB_SIZE; i ++)
        shell.refresh_list->keys[i] = 0;
    
    shell_port_io_init();
    shell_port_set_recv_evt(Evt_Shell_IoRecv);

    // 特殊功能键回调函数
    shell.fkey_hook = shell_fkey_hook_null;
    shell.fkey_hook_default = shell_fkey_hook_null;
    
    // assert
    shell.name_module = (void *)0;
    shell.line_num = 0;

    shell.en_log = false;

    // Shell
    obj_init(&shell.super, "Shell", ObjType_Sm, ObjPrio_Shell, 20);
    sm_start(&shell.super, STATE_CAST(state_init));
}

void shell_start(void)
{
    evt_publish(Evt_Shell_Ready);
    const char * str_start = "\033[0;37m>> \n\rShell Started !\n\r>> ";
    shell_port_send((void *)str_start, strlen(str_start));
    
    shell_port_default_func_reg();
}

void shell_mode(int mode)
{
    if (shell.mode == ShellMode_Act && mode != ShellMode_Act) {
        evt_publish_para(Evt_Shell_ModeSwitch, &mode);
    }
    if (shell.mode != ShellMode_Act && mode == ShellMode_Act)
        evt_publish_para(Evt_Shell_ModeSwitch, &mode);
}

void shell_cmd_reg(const char * cmd, shell_hook_t hook, const char * help)
{
    int i = shell.count_cmd;
    shell.cmd_table[i].name = cmd;
    shell.cmd_table[i].hook = hook;
    shell.cmd_table[i].help = help;
    shell.count_cmd ++;
}

void shell_db_refresh(const char * key)
{
    sh_refresh_data_t * refresh = shell.refresh_list;
    while (refresh != (void *)0) {
        for (int i = 0; i < SHELL_REFRESH_TAB_SIZE; i ++) {
            if (refresh->keys[i] == 0) {
                refresh->keys[i] = key;
                refresh->value = 0;
                return;
            }
        }

        refresh = refresh->next;
    }

    refresh = shell_port_malloc(sizeof(sh_refresh_data_t));
    M_ASSERT(refresh != (void *)0);

    // 加入链表
    refresh->next = shell.refresh_list;
    shell.refresh_list = refresh;
    for (int i = 0; i < SHELL_REFRESH_TAB_SIZE; i ++)
        refresh->keys[i] = 0;
    refresh->keys[0] = key;
}

void shell_set_fkey_hook(shell_fkey_hook_t fkey_hook)
{
    shell.fkey_hook = fkey_hook;
}

int32_t shell_print(const char * format, ...)
{
    int _ret;
    va_list para_list;

    for (int i = 0; i < SHELL_BUFF_SEND_SIZE; i ++)
        shell.buff_send[i] = 0;
    va_start(para_list, format);
    _ret = vsprintf ((char *)shell.buff_send, format, para_list);
    va_end(para_list);
    
    int length = strlen((char *)shell.buff_send);
    if (length > 0) {
        shell_port_send(shell.buff_send, length);
    }

    return _ret;
}

// prtivate state - sm_shell ---------------------------------------------------
static const uint32_t evt_sub_table[] = {
    Evt_Shell_Key,
    Evt_Shell_ModeSwitch,
    Evt_Shell_IoRecv,
    Evt_Shell_Ready,
};
static m_ret_t state_init(shell_t * const me, m_evt_t const * const e)
{
    (void)e;

    for (int i = 0; i < sizeof(evt_sub_table) / sizeof(uint32_t); i ++)
        evt_subscribe(&(me->super), evt_sub_table[i]);

    return M_TRAN(state_unready);
}

static m_ret_t state_unready(shell_t * const me, m_evt_t const * const e)
{
    switch (e->sig) {
        case Evt_Shell_Ready:
            return M_TRAN(state_work);
        
        default:
            return M_SUPER(m_state_top);
    }
}

static m_ret_t state_work(shell_t * const me, m_evt_t const * const e)
{
    if (e->sig == Evt_Shell_IoRecv) {

        uint8_t _count = shell_port_recv(me->buff_esc, 16);
        if (_count == 0) {
            return M_Ret_Handled;
        }
        // 普通按键
        if (_count == 1 && me->buff_esc[0] != Key_Esc) {
            me->count_esc = 1;
            shell_publish_key(me->buff_esc[0]);
            fflush(stdout);
        }
        // ESC键和特殊功能键
        else if (me->buff_esc[0] == Key_Esc) {
            sh_key_value_func(me->buff_esc, _count);
        }
        else if (_count > 1 && me->buff_esc[0] != Key_Esc) {
            uint16_t i;
            for (i = 0; i < _count; i ++)
                if (me->buff_esc[i] != Key_Esc)
                    shell_publish_key(me->buff_esc[i]);
                else
                    break;
            for (uint8_t j = i; j < _count; j ++)
                me->buff_esc[j - i] = me->buff_esc[j];
            me->count_esc = _count - i;
        }
        else
            M_ASSERT(false);

        return M_Ret_Handled;
    }

    switch (e->sig) {
        case Evt_Init:
            return M_TRAN(state_act);

        default:
            return M_SUPER(m_state_top);
    }
}

static m_ret_t state_act(shell_t * const me, m_evt_t const * const e)
{
    switch (e->sig) {
        case Evt_Enter: {
            me->mode = ShellMode_Act;
            evt_subscribe(&(me->super), Evt_Time_50ms);
            const char * string = "\n\r\033[0;37m>>\033[1;33m Shell Act Mode.\033[0m\n\r>> ";
            shell_port_send((void *)string, strlen(string));
            return M_Ret_Handled;
        }

        case Evt_Exit:
            evt_unsubscribe(&(me->super), Evt_Time_50ms);
            return M_Ret_Handled;

        case Evt_Shell_Key: {
            uint8_t _key_id = (uint8_t)e->para[0].u32;
            // 指令字符
            if (sh_is_cmd_char(_key_id) == true && me->count_cmd_char < SHELL_BUFF_CMD_SIZE) {
                shell_port_send((void *)&_key_id, 1);
                me->cmd_buffer[me->count_cmd_char] = _key_id;
                me->count_cmd_char ++;
            }
            // 退格键
            else if (_key_id == Key_BackSpace || _key_id == 127) {
                if (me->count_cmd_char > 0) {
                    me->count_cmd_char --;
                    me->cmd_buffer[me->count_cmd_char] = 0;
                    if (me->en_log == false)
                        shell_port_send((void *)"\b \b", 3);
                }
            }
            // 回车键
            else if (_key_id == Key_Enter || _key_id == Key_NewLine) {
                shell_cmd_parser(me);
            }
            // 特殊功能键
            else {
                if (_key_id == fkey_data[Key_Up].id)
                    shell_cmd_history(me, true);
                else if (_key_id == fkey_data[Key_Down].id)
                    shell_cmd_history(me, false);
                return M_Ret_Handled;
            }
            return M_Ret_Handled;
        }

        case Evt_Shell_ModeSwitch: {
            int mode = e->para[0].s32;
            if (mode == ShellMode_Refresh)
                return M_TRAN(state_refresh);
            else if (mode == ShellMode_Log)
                return M_TRAN(state_log);
            else if (mode == ShellMode_Scroll)
                return M_TRAN(state_scroll);
            else if (mode == ShellMode_FileRecv) {
                return M_TRAN(state_filerecv);
            }
            return M_Ret_Handled;
        }

        default:
            return M_SUPER(state_work);
    }
}

// TODO 对于Refresh的实现，不适用mdb_get_value_int如何实现。
static int count_char_pos = 0;              // 光标位置
static int count_keys_refresh = 0;
char value_char[21];                        // value显示缓冲区
char value_former[10];
static m_ret_t state_refresh(shell_t * const me, m_evt_t const * const e)
{
    switch (e->sig) {
        case Evt_Enter: {
            count_keys_refresh = 0;
            me->mode = ShellMode_Refresh;
            evt_subscribe(&(me->super), Evt_Time_500ms);
            const char * string = "\n\r\033[0;37m>>\033[1;33m Shell Refresh Mode.\033[0m\n\r>> \n\r";
            shell_port_send((void *)string, strlen(string));
            // 显示需要刷新的Key
            sh_refresh_data_t * refresh = me->refresh_list;
            for (int i = 0; i < SHELL_REFRESH_TAB_SIZE; i ++) {
                if (refresh->keys[i] == 0)
                    continue;
                int value = 0;
                if (mdb_get_value_int(refresh->keys[i], &value) != MdbRet_OK)
                    continue;
                shell_port_send((void *)" + ", 3);
                shell_port_send((void *)refresh->keys[i], strlen(refresh->keys[i]));
                shell_port_send((void *)"\n\r", 2);
                count_char_pos = 3 + strlen(refresh->keys[i]) + 2;
                count_keys_refresh ++;
            }
            // 光标移动到第40个字符处
            for (int i = 0; i < 40 - count_char_pos; i ++)
                shell_port_send((void *)" ", 1);
            return M_Ret_Handled;
        }

        case Evt_Exit: {
            evt_unsubscribe(&(me->super), Evt_Time_500ms);
            shell_port_send((void *)"\n\r", 2);
            // 清除刷新显示的Keys
            sh_refresh_data_t * refresh = shell.refresh_list;
            while (refresh != (void *)0) {
                for (int i = 0; i < SHELL_REFRESH_TAB_SIZE; i ++)
                    refresh->keys[i] = 0;
                refresh = refresh->next;
            }
            return M_Ret_Handled;
        }

        case Evt_Shell_Key: {
            uint8_t _key_id = e->para[0].u32;
            if (_key_id == Key_Esc)
                return M_TRAN(state_act);
            uint8_t fkey_id = shell_get_fkey_id(_key_id);
            if (fkey_id != 0xff)
                me->fkey_hook(fkey_id);
            return M_Ret_Handled;
        }
        
        case Evt_Time_500ms: {
            // 返回最顶端
            uint8_t up_to_origin[3] = { 0x1b, 0x5b, 0x41 };
            for (int i = 0; i < count_keys_refresh; i ++) {
                shell_port_send((void *)up_to_origin, 3);
            }
            // 刷新值
            sh_refresh_data_t * refresh = me->refresh_list;
            
            
            while (refresh != (void *)0) {
                for (int i = 0; i < SHELL_REFRESH_TAB_SIZE; i ++) {
                    if (refresh->keys[i] == 0)
                        continue;
                    int value = 0;
                    if (mdb_get_value_int(refresh->keys[i], &value) != MdbRet_OK)
                        continue;
                    for (int i = 0; i < 21; i ++)
                        value_char[i] = 0;
                    // 值的字符
                    int length = sprintf(value_char, "%d", value);
                    // 将其余写空格
                    for (int m = length; m < 10; m ++)
                        value_char[m] = ' ';
                    for (int m = 10; m < 21; m ++)
                        value_char[m] = 0x08;
                    value_char[20] = 0x0c;
                    shell_port_send((void *)value_char, strlen(value_char));
                }
                refresh = refresh->next;
            }
            return M_Ret_Handled;
        }

        default:
            return M_SUPER(state_work);
    }
}

static m_ret_t state_log(shell_t * const me, m_evt_t const * const e)
{
    switch (e->sig) {
        case Evt_Enter:
            me->mode = ShellMode_Log;
            evt_subscribe(&(me->super), Evt_Time_500ms);
            return M_Ret_Handled;

        case Evt_Exit:
            evt_unsubscribe(&(me->super), Evt_Time_500ms);
            shell_port_send((void *)"\n\r", 2);
            return M_Ret_Handled;            

        case Evt_Shell_Key: {
            uint8_t _key_id = e->para[0].u32;
            if (_key_id == Key_Esc)
                return M_TRAN(state_act);
            return M_Ret_Handled;
        }

        case Evt_Time_500ms:
            
            return M_Ret_Handled;

        default:
            return M_SUPER(state_work);
    }
}

static m_ret_t state_scroll(shell_t * const me, m_evt_t const * const e)
{
    switch (e->sig) {
        case Evt_Enter:
            me->mode = ShellMode_Scroll;
            evt_subscribe(&(me->super), Evt_Time_500ms);
            return M_Ret_Handled;

        case Evt_Exit:
            evt_unsubscribe(&(me->super), Evt_Time_500ms);
            shell_port_send((void *)"\n\r", 2);
            return M_Ret_Handled;            

        case Evt_Shell_Key: {
            uint8_t _key_id = e->para[0].u32;
            if (_key_id == Key_Esc)
                return M_TRAN(state_act);
            return M_Ret_Handled;
        }

        default:
            return M_SUPER(state_work);
    }
}

static m_ret_t state_filerecv(shell_t * const me, m_evt_t const * const e)
{
    switch (e->sig) {
        case Evt_Enter:
            evt_unsubscribe(&(me->super), Evt_Shell_IoRecv);
            return M_Ret_Handled;

        case Evt_Exit:
            evt_subscribe(&(me->super), Evt_Shell_IoRecv);
            return M_Ret_Handled;

        case Evt_Shell_ModeSwitch: {
            int mode = e->para[0].s32;
            if (mode == ShellMode_Act)
                return M_TRAN(state_act);
            return M_Ret_Handled;
        }

        default:
            return M_SUPER(state_work);
    }
}

// private function ------------------------------------------------------------
static bool sh_is_cmd_char(uint8_t cmd_char)
{
    if (cmd_char >= '0' && cmd_char <= '9')
        return true;
    else if (cmd_char >= 'a' && cmd_char <= 'z')
        return true;
    else if (cmd_char >= 'A' && cmd_char <= 'Z')
        return true;
    else if (cmd_char == Key_Space)
        return true;
    else if (cmd_char == '-')
        return true;
    else if (cmd_char == '_')
        return true;
    else
        return false;
}

static bool sh_is_num_and_char(uint8_t cmd_char)
{
    if (cmd_char >= '0' && cmd_char <= '9')
        return true;
    else if (cmd_char >= 'a' && cmd_char <= 'z')
        return true;
    else if (cmd_char >= 'A' && cmd_char <= 'Z')
        return true;
    else
        return false;
}

static void sh_key_value_func(uint8_t * p_data, uint8_t count)
{
    M_ASSERT(count <= 128);
    
    uint8_t key_id_f1 = 201;
    uint8_t key_id_f5 = 220;
    uint8_t key_id_f9 = 215;
    uint8_t key_id_up = 205;
    uint8_t key_id_home = 209;

    uint8_t _count = count;

    uint8_t _buff_esc[128];
    for (uint8_t i = 0; i < count; i ++) {
        _buff_esc[i] = *(p_data + i);
    }

    // normal/esc
    if (count == 1) {
        shell_publish_key(_buff_esc[0]);
        return;
    }
    // key (normal + esc) or (esc + normal) or (normal + normal)
    // note: when esc + normal, drop the normal key
    else if (count == 2) {
        if (_buff_esc[0] == Key_Esc && _buff_esc[1] != Key_Esc)
            shell_publish_key(Key_Esc);
        else {
            shell_publish_key(_buff_esc[0]);
            shell_publish_key(_buff_esc[1]);
        }
        return;
    }
    else if (count > 5) {
        if (_buff_esc[0] == Key_Esc && _buff_esc[1] == Key_Esc)
            return;
        else if (_buff_esc[0] == Key_Esc && _buff_esc[1] != Key_Esc) {
            if (_buff_esc[3] == Key_Esc)
                _count = 3;
            else if (_buff_esc[4] == Key_Esc)
                _count = 4;
            else if (_buff_esc[5] == Key_Esc)
                _count = 5;
        }
    }
    
    uint8_t _key_value = 0;
    // one special key(3 bytes) or 3 (normal/esc)
    if (_count == 3) {
        if (_buff_esc[0] == Key_Esc) {
            if (*(p_data + 1) == 0x4F
                && *(p_data + 2) >= 0x50 && *(p_data + 2) <= 0x53) {
                _key_value = key_id_f1 + *(p_data + 2) - 0x50;
            }
            else if (*(p_data + 1) == 0x5B
                     && *(p_data + 2) >= 0x41 && *(p_data + 2) <= 0x44) {
                _key_value = key_id_up + *(p_data + 2) - 0x41;
            }
            else
                return;
            shell_publish_key(_key_value);
        }
        else {
            shell_publish_key(_buff_esc[0]);
            shell_publish_key(_buff_esc[1]);
            shell_publish_key(_buff_esc[2]);
        }
        return;
    }
    // one special key(3 bytes) + normal/esc or 
    // one special key(4 bytes)
    // drop 4 (normal/esc)
    else if (_count == 4) {
        if (_buff_esc[1] == Key_Esc) {
            if (*(p_data + 2) == 0x4F
                && *(p_data + 3) >= 0x50 && *(p_data + 3) <= 0x53) {
                _key_value = key_id_f1 + *(p_data + 3) - 0x50;
            }
            else if (*(p_data + 2) == 0x5B
                     && *(p_data + 3) >= 0x41 && *(p_data + 3) <= 0x44) {
                _key_value = key_id_up + *(p_data + 3) - 0x41;
            }
            else
                return;
            shell_publish_key(_buff_esc[0]);
            shell_publish_key(_key_value);
        }
        else if (_buff_esc[0] == Key_Esc && _buff_esc[1] != Key_Esc) {
            if (*(p_data + 1) == 0x5B && *(p_data + 3) == 0x7E
                && *(p_data + 2) >= 0x31 && *(p_data + 2) <= 0x36) {
                _key_value = key_id_home + *(p_data + 2) - 0x31;
                shell_publish_key(_key_value);
            }
            else if (*(p_data + 1) == 0x4F
                && *(p_data + 2) >= 0x50 && *(p_data + 2) <= 0x53) {
                _key_value = key_id_f1 + *(p_data + 2) - 0x50;
                shell_publish_key(_key_value);
                shell_publish_key(_buff_esc[3]);
            }
            else if (*(p_data + 1) == 0x5B
                     && *(p_data + 2) >= 0x41 && *(p_data + 2) <= 0x44) {
                _key_value = key_id_up + *(p_data + 2) - 0x41;
                shell_publish_key(_key_value);
                shell_publish_key(_buff_esc[3]);
            }
        }
        return;
    }
    else if (_count == 5) {
        if (*p_data == 0x1B
            && *(p_data + 1) == 0x5B && *(p_data + 2) == 0x32
            && *(p_data + 4) == 0x7E && *(p_data + 3) >= 0x30
            && *(p_data + 3) <= 0x34 && *(p_data + 3) != 0x32) {
            _key_value = key_id_f9 + *(p_data + 3) - 0x30;
        }
        else if (*p_data == 0x1B
                 && *(p_data + 1) == 0x5B && *(p_data + 2) == 0x31
                 && *(p_data + 4) == 0x7E && *(p_data + 3) >= 0x35
                 && *(p_data + 3) <= 0x39 && *(p_data + 3) != 0x36) {
            _key_value = key_id_f5 + *(p_data + 3) - 0x35;
        }
        else
            return;
        shell_publish_key(_key_value);
        return;
    }
    else
        M_ASSERT(false);
}

static void shell_publish_key(uint8_t key_id)
{
    int para = key_id;
    evt_publish_para(Evt_Shell_Key, &para);
    if (key_id == Key_Esc)
        evt_publish(Evt_Shell_Esc);
    else if (key_id == fkey_data[Key_F1].id)
        evt_publish(Evt_Shell_F1);
}

static void shell_cmd_history(shell_t * const me, bool history_up)
{
    if (me->cmd_histroy_empty == true)
        return;
    // 未翻到历史指令，向上翻
    if (history_up == true && me->head_history_temp == INT32_MAX) {
        me->head_history_temp = me->head_history;
        // 备份当前指令
        for (int i = 0; i < me->count_cmd_char; i ++)
            me->cmd_buffer_bkp[i] = me->cmd_buffer[i];
        me->cmd_buffer_bkp[me->count_cmd_char] = 0;
    }
    // 已经翻到历史指令，向上翻，则寻找上一个
    else if (history_up == true && me->head_history_temp != INT32_MAX) {
        // 前一个为0，当前格不为0，就是上一个历史指令
        int index_crt = me->head_history_temp, index_last;
        do {
            index_crt = index_crt == 0 ? SHELL_BUFF_HISTORY_SIZE - 1: index_crt - 1;
            index_last = index_crt == 0 ? SHELL_BUFF_HISTORY_SIZE - 1: index_crt - 1;
        } while (!(me->cmd_history[index_crt] != 0 && me->cmd_history[index_last] == 0));
        // 如果循环找到了最新的历史指令，则不变
        if (me->head_history != index_crt)
            me->head_history_temp = index_crt;
    }
    // 未翻到历史指令，向下翻，不做任何变化
    else if (history_up == false && me->head_history_temp == INT32_MAX) {
        return;
    }
    // 翻到最新的历史指令，向下翻，恢复之前输入的指令
    else if (history_up == false && me->head_history_temp == me->head_history) {
        // 将已经打印出来的指令消除掉
        for (int i = 0; i < me->count_cmd_char; i ++)
            shell_port_send((void *)"\b \b", 3);
        me->head_history_temp = INT32_MAX;
        // 恢复之前备份的指令
        me->count_cmd_char = strlen(me->cmd_buffer_bkp);
        for (int i = 0; i < me->count_cmd_char; i ++)
            me->cmd_buffer[i] = me->cmd_buffer_bkp[i];
        me->cmd_buffer[me->count_cmd_char] = 0;
        // 打印此历史指令
        shell_port_send((void *)me->cmd_buffer, me->count_cmd_char);
        return;
    }
    // 翻到非最新的历史指令，向下翻，则寻找下一个
    else if (history_up == false && me->head_history_temp != me->head_history) {
        // 当前格为0，下个格不为0，就是下一个历史指令
        int index_crt = me->head_history_temp, index_next;
        do {
            index_crt = (index_crt + 1) % SHELL_BUFF_HISTORY_SIZE;
            index_next = (index_crt + 1) % SHELL_BUFF_HISTORY_SIZE;
        } while (!(me->cmd_history[index_crt] == 0 && me->cmd_history[index_next] != 0));
        // 如果循环找到了最新的历史指令，则不变
        me->head_history_temp = index_next;
    }

    // 将已经打印出来的指令消除掉
    for (int i = 0; i < me->count_cmd_char; i ++)
        shell_port_send((void *)"\b \b", 3);
    // 获取历史指令的长度
    int length_history = 0;
    int i = me->head_history;
    while (me->cmd_history[i] != 0) {
        length_history ++;
        i = ((i + 1) % SHELL_BUFF_HISTORY_SIZE);
    }
    // 将历史指令填入Buffer
    int m = me->head_history_temp;
    me->count_cmd_char = length_history;
    for (int i = 0; i < me->count_cmd_char; i ++) {
        me->cmd_buffer[i] = me->cmd_history[(m + i) % SHELL_BUFF_HISTORY_SIZE];
    }
    me->cmd_buffer[me->count_cmd_char] = 0;
    // 打印此历史指令
    shell_port_send((void *)me->cmd_buffer, me->count_cmd_char);
}

static void shell_cmd_parser(shell_t * const me)
{
    // 如果没有指令码
    if (me->count_cmd_char == 0) {
        shell_port_send((void *)data_newline, 5);
        return;
    }

    me->cmd_buffer[me->count_cmd_char + 1] = 0;

    if (me->cmd_histroy_empty == true) {
        me->cmd_histroy_empty = false;
        for (int i = 0; i < me->count_cmd_char; i ++)
            me->cmd_history[i] = me->cmd_buffer[i];
        me->cmd_history[me->count_cmd_char] = 0;
    }
    // 与历史指令对比，若不同，加入进去
    else {
        // 检查与历史指令是否相同，如相同，直接返回
        if (strcmp(me->cmd_buffer, &me->cmd_history[me->head_history]) == 0)
            goto SHELL_CMD_ARRANGE;
        // 获取最新历史指令的长度
        int length_history = 0;
        int i = me->head_history;
        while (me->cmd_history[i] != 0) {
            length_history ++;
            i = ((i + 1) % SHELL_BUFF_HISTORY_SIZE);
        }
        // 清理出保存此指令的空间
        me->head_history = (me->head_history + length_history + 1) % SHELL_BUFF_HISTORY_SIZE;
        int next_head_history = (me->head_history + me->count_cmd_char + 1) % SHELL_BUFF_HISTORY_SIZE;
        // 如果清理的空间没有分布在接缝处
        if (me->head_history < next_head_history) {
            for (int i = me->head_history, j = 0; i < next_head_history; i ++, j ++)
                me->cmd_history[i] = me->cmd_buffer[j];
        }
        else {
            int j = 0;
            for (int i = me->head_history; i < SHELL_BUFF_HISTORY_SIZE; i ++, j ++)
                me->cmd_history[i] = me->cmd_buffer[j];
            for (int i = 0; i < next_head_history; i ++, j ++)
                me->cmd_history[i] = me->cmd_buffer[j];
        }
        // 清零掉刚才未清理完的指令尾巴
        i = next_head_history;
        while (me->cmd_history[i] != 0) {
            me->cmd_history[i] = 0;
            i = ((i + 1) % SHELL_BUFF_HISTORY_SIZE);
        }
    }

SHELL_CMD_ARRANGE:
    me->head_history_temp = INT32_MAX;

    // 整理指令Buffer
    char cmd[me->count_cmd_char + 1];
    int count_cmd = me->count_cmd_char + 1;
    for (int i = 0; i < me->count_cmd_char + 1; i ++)
        cmd[i] = 0;
    // 拷贝指令，并将所有的空格变为0.
    for (int i = 0; i < me->count_cmd_char; i ++) {
        cmd[i] = me->cmd_buffer[i];
        if (cmd[i] == ' ')
            cmd[i] = 0;
    }

    #define SHELL_CMD_MODE_SIZE                 16
    #define SHELL_CMD_PARA_SIZE                 4

    // 解析指令的各个元素
    char * name = 0, * function = 0, * para[SHELL_CMD_PARA_SIZE];
    char mode[SHELL_CMD_MODE_SIZE];
    for (int i = 0; i < SHELL_CMD_PARA_SIZE; i ++)
        para[i] = 0;
    for (int i = 0; i < SHELL_CMD_MODE_SIZE; i ++)
        mode[i] = 0;
    int count_mode = 0;
    int count_para = 0;
    // 指令名称 例如mdb
    name = &cmd[0];
    // function --help --version之类
    for (int i = strlen(name); i < count_cmd - 3; i ++) {
        if (cmd[i] == 0 && cmd[i + 1] == '-' && cmd[i + 2] == '-' && cmd[i + 3] != '-') {
            function = &cmd[i + 1];
            goto SHELL_CMD_PARSER;
        }
    }
    // 模式 -rf -m -a之类
    for (int i = strlen(name); i < count_cmd - 1; i ++) {
        if (cmd[i] == 0 && cmd[i + 1] == '-') {
            int m = 0;
            while (cmd[i + 2 + m] != 0)
                mode[count_mode ++] = cmd[i + 2 + m ++];
            if (count_mode >= SHELL_CMD_MODE_SIZE) {
                const char * info = "Wrong Cmd. Too many -mode.\n\r>> ";
                shell_port_send((void *)info, strlen(info));
                sh_clear_buff_cmd();
                return;
            }
        }
    }
    // 参数
    for (int i = strlen(name); i < count_cmd - 1; i ++) {
        if (cmd[i] == 0 && sh_is_num_and_char(cmd[i + 1]) == true) {
            para[count_para] = &cmd[i + 1];
            if (count_para >= SHELL_CMD_PARA_SIZE) {
                const char * info = "Wrong Cmd. Too many para.\n\r>> ";
                shell_port_send((void *)info, strlen(info));
                sh_clear_buff_cmd();
                return;
            }
            
            count_para ++;
        }
    }
    
    // 解析
    uint16_t cmd_num = sizeof(sh_cmdtable_def) / sizeof(sh_cmd_data_t);

SHELL_CMD_PARSER:
    // 在默认列表中的指令
//    for (int i = 0; i < cmd_num; i ++) {
//        if (strcmp(name, sh_cmdtable_def[i].name) != 0)
//            continue;

//        shell_port_send((void *)data_newline, 5);
//        sh_cmdtable_def[i].hook(mode, function, count_para, para);
//        // clear all cmd char
//        sh_clear_buff_cmd();
//        return;
//    }

    // 在指令注册列表中的指令
    for (int m = 0; m < me->count_cmd; m ++) {
        if (strcmp(name, me->cmd_table[m].name) != 0)
            continue;
        // 一个新行
        shell_port_send((void *)data_newline, 5);
        
        shell_port_send((void *)data_newline, 5);
        me->cmd_table[m].hook(mode, function, count_para, para);
        // clear all cmd char
        sh_clear_buff_cmd();
        return;
    }

    // 错误指令
    // a new line
    shell_port_send((void *)data_newline, 5);
    
    const char * wrong_cmd = "wrong cmd!";
    shell_port_send((void *)wrong_cmd, strlen(wrong_cmd));
    // clear all cmd char
    sh_clear_buff_cmd();
    // a new line
    shell_port_send(data_newline, 5);
}

static void sh_clear_buff_cmd(void)
{
    // clear all cmd char
    for (uint16_t i = 0; i < SHELL_BUFF_CMD_SIZE; i ++)
        shell.cmd_buffer[i] = 0;
    shell.count_cmd_char = 0;
}

static void shell_hook_null(char * mode, char * func, int num, char ** para)
{
    (void)mode; (void)func; (void)num; (void)para;
}

static void shell_fkey_hook_null(uint8_t key_id)
{
    (void)key_id;
}

static void shell_fkey_hook_defualt(uint8_t key_id)
{
}

static uint8_t shell_get_fkey_id(int key_id)
{
    for (int i = 0; i < sizeof(fkey_data) / sizeof(shell_fkey_data_t); i ++) {
        if (fkey_data[i].id == key_id)
            return i;
    }
    
    return 0xff;
}

// hook ------------------------------------------------------------------------
static void hook_help(char * mode, char * func, int num, char ** para)
{
    const char * string;
    uint8_t newline[2] = {0x0D, 0x0A};
    string = "Shell help:\n\r";
    shell_port_send((void *)string, strlen(string));
    
    int cmd_num = sizeof(sh_cmdtable_def) / sizeof(sh_cmd_data_t);
    for (int i = 0; i < cmd_num; i ++) {
        shell_port_send("  + ", 4);
        shell_port_send((void *)sh_cmdtable_def[i].help,
                        strlen(sh_cmdtable_def[i].help));
        shell_port_send(newline, 2);
    }
    for (int i = 0; i < shell.count_cmd; i ++) {
        shell_port_send((void *)shell.cmd_table[i].help,
                        strlen(shell.cmd_table[i].help));
        shell_port_send(newline, 2);
    }
    shell_port_send(">> ", 3);
}
