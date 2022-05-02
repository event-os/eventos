
// include ---------------------------------------------------------------------
#include "eventos.h"
#include "esh.h"
#include <stdarg.h>
#include <string.h>

EOS_TAG("E-Shell")

#define ESH_MUTEX               "mutex_esh"

// key value -------------------------------------------------------------------
enum
{
    Esh_Backspace = Esh_Max,
    Esh_NewLine,
    Esh_Enter,
    Esh_Esc,
    Esh_Delete,

    Esh_MaxMax
};

typedef struct esh_key_info
{
    uint8_t key;
    uint8_t length;
#if (EOS_SHELL_CLIENT == 1)
    uint8_t value[2];
#elif (EOS_SHELL_CLIENT == 2)
    uint8_t value[5];
#endif
    const char * name;
} esh_key_info_t;

// 特殊功能键
#if (EOS_SHELL_CLIENT == 1)
static const esh_key_info_t fkey_value_table[] =
{
    { Esh_Null,         0, { 0x00, 0x00 }, "Null" },
    { Esh_F1,           2, { 0x00, 0x3B }, "F1" },
    { Esh_F2,           2, { 0x00, 0x3C }, "F2" },
    { Esh_F3,           2, { 0x00, 0x3D }, "F3" },
    { Esh_F4,           2, { 0x00, 0x3E }, "F4" },
    { Esh_F5,           2, { 0x00, 0x3F }, "F5" },
    { Esh_F6,           2, { 0x00, 0x40 }, "F6" },
    { Esh_F7,           2, { 0x00, 0x41 }, "F7" },
    { Esh_F8,           2, { 0x00, 0x42 }, "F8" },
    { Esh_F9,           2, { 0x00, 0x43 }, "F9" },
    { Esh_F10,          2, { 0x00, 0x44 }, "F10" },
    { Esh_F12,          2, { 0xE0, 0x86 }, "F12" },
    { Esh_Up,           2, { 0xE0, 0x48 }, "Up" },
    { Esh_Down,         2, { 0xE0, 0x50 }, "Down"  },
    { Esh_Left,         2, { 0xE0, 0x4B }, "Left" },
    { Esh_Right,        2, { 0xE0, 0x4D }, "Right" },
    { Esh_Home,         2, { 0xE0, 0x47 }, "Home" },
    { Esh_Insert,       2, { 0xE0, 0x52 }, "Insert" },
    { Esh_Delect,       2, { 0xE0, 0x53 }, "Delect" },
    { Esh_End,          2, { 0xE0, 0x4F }, "End" },
    { Esh_PageUp,       2, { 0xE0, 0x49 }, "PageUp" },
    { Esh_PageDown,     2, { 0xE0, 0x51 }, "PageDown" },
};
#endif

#if (EOS_SHELL_CLIENT == 2)
static const esh_key_info_t fkey_value_table[] = {
    { Esh_Null,         0, { 0x00, 0x00, 0x00, 0x00, 0x00 }, "Null" },
    { Esh_F1,           3, { 0x1B, 0x4F, 0x50, 0x00, 0x00 }, "F1" },
    { Esh_F2,           3, { 0x1B, 0x4F, 0x51, 0x00, 0x00 }, "F2" },
    { Esh_F3,           3, { 0x1B, 0x4F, 0x52, 0x00, 0x00 }, "F3" },
    { Esh_F4,           3, { 0x1B, 0x4F, 0x53, 0x00, 0x00 }, "F4" },
    { Esh_F5,           5, { 0x1B, 0x5B, 0x31, 0x35, 0x7E }, "F5" },
    { Esh_F6,           5, { 0x1B, 0x5B, 0x31, 0x37, 0x7E }, "F6" },
    { Esh_F7,           5, { 0x1B, 0x5B, 0x31, 0x38, 0x7E }, "F7" },
    { Esh_F8,           5, { 0x1B, 0x5B, 0x31, 0x39, 0x7E }, "F8" },
    { Esh_F9,           5, { 0x1B, 0x5B, 0x32, 0x30, 0x7E }, "F9" },
    { Esh_F10,          5, { 0x1B, 0x5B, 0x32, 0x31, 0x7E }, "F10" },
    { Esh_F11,          5, { 0x1B, 0x5B, 0x32, 0x33, 0x7E }, "F11" },
    { Esh_F12,          5, { 0x1B, 0x5B, 0x32, 0x34, 0x7E }, "F12" },
    { Esh_Up,           3, { 0x1B, 0x5B, 0x41, 0x00, 0x00 }, "Up" },
    { Esh_Down,         3, { 0x1B, 0x5B, 0x42, 0x00, 0x00 }, "Down"  },
    { Esh_Left,         3, { 0x1B, 0x5B, 0x43, 0x00, 0x00 }, "Left" },
    { Esh_Right,        3, { 0x1B, 0x5B, 0x44, 0x00, 0x00 }, "Right" },
    { Esh_Home,         4, { 0x1B, 0x5B, 0x31, 0x7E, 0x00 }, "Home" },
    { Esh_Insert,       4, { 0x1B, 0x5B, 0x32, 0x7E, 0x00 }, "Insert" },
    { Esh_Delect,       4, { 0x1B, 0x5B, 0x33, 0x7E, 0x00 }, "Delect" },
    { Esh_End,          4, { 0x1B, 0x5B, 0x34, 0x7E, 0x00 }, "End" },
    { Esh_PageUp,       4, { 0x1B, 0x5B, 0x35, 0x7E, 0x00 }, "PageUp" },
    { Esh_PageDown,     4, { 0x1B, 0x5B, 0x36, 0x7E, 0x00 }, "PageDown" },
};
#endif

// normal key
static const esh_key_info_t nkey_value_table[] =
{
    { Esh_Backspace,    1, { 0x08, }, "Backspace" },
    { Esh_NewLine,      1, { 0x0A, }, "NewLine" },
    { Esh_Enter,        1, { 0x0D, }, "Enter" },
    { Esh_Esc,          1, { 0x1B, }, "Esc" },
    { Esh_Delete,       1, { 0x7F, }, "Delete" },
};


// 组合键
static const esh_key_info_t zkey_value_table[] =
{
#if (EOS_SHELL_CLIENT == 2)
    { Esh_Ctrl_A,       1, { 0x01, }, "Ctrl_A" },
#endif
    { Esh_Ctrl_B,       1, { 0x02, }, "Ctrl_B" },
#if (EOS_SHELL_CLIENT == 2)
    { Esh_Ctrl_C,       1, { 0x03, }, "Ctrl_C" },
#endif
    { Esh_Ctrl_D,       1, { 0x04, }, "Ctrl_D" },
    { Esh_Ctrl_E,       1, { 0x05, }, "Ctrl_E" },
#if (EOS_SHELL_CLIENT == 2)
    { Esh_Ctrl_F,       1, { 0x06, }, "Ctrl_F" },
#endif
    { Esh_Ctrl_G,       1, { 0x07, }, "Ctrl_G" },
#if (EOS_SHELL_CLIENT == 1)
    { Esh_Ctrl_H,       1, { 0x08, }, "Ctrl_H" },
#endif
    { Esh_Ctrl_I,       1, { 0x09, }, "Ctrl_I" },
#if (EOS_SHELL_CLIENT == 1)
    { Esh_Ctrl_J,       1, { 0x0A, }, "Ctrl_J" },
#endif
    { Esh_Ctrl_K,       1, { 0x0B, }, "Ctrl_K" },
    { Esh_Ctrl_L,       1, { 0x0C, }, "Ctrl_L" },
#if (EOS_SHELL_CLIENT == 0)
    { Esh_Ctrl_M,       1, { 0x0D, }, "Ctrl_M" },
#endif
#if (EOS_SHELL_CLIENT == 2)
    { Esh_Ctrl_N,       1, { 0x0E, }, "Ctrl_N" },
#endif
    { Esh_Ctrl_O,       1, { 0x0F, }, "Ctrl_O" },
    { Esh_Ctrl_P,       1, { 0x10, }, "Ctrl_P" },
    { Esh_Ctrl_Q,       1, { 0x11, }, "Ctrl_Q" },
    { Esh_Ctrl_R,       1, { 0x12, }, "Ctrl_R" },
    { Esh_Ctrl_S,       1, { 0x13, }, "Ctrl_S" },
    { Esh_Ctrl_T,       1, { 0x14, }, "Ctrl_T" },
    { Esh_Ctrl_U,       1, { 0x15, }, "Ctrl_U" },
#if (EOS_SHELL_CLIENT == 2)
    { Esh_Ctrl_V,       1, { 0x16, }, "Ctrl_V" },
#endif
    { Esh_Ctrl_W,       1, { 0x17, }, "Ctrl_W" },
    { Esh_Ctrl_X,       1, { 0x18, }, "Ctrl_X" },
    { Esh_Ctrl_Y,       1, { 0x19, }, "Ctrl_Y" },
    { Esh_Ctrl_Z,       1, { 0x1A, }, "Ctrl_Z" },
};


// 特殊功能键数据 ---------------------------------------------------------------
// Msh状态
enum
{
    EshState_Unready = 0,
    EshState_Act,
    EshState_Log,

    EshState_Max
};

// data struct -----------------------------------------------------------------
// 指令数据
typedef struct esh_cmd_reg
{
    const char * name;                              // 指令名称
    uint8_t shortcut;                               // 快捷键
    esh_hook_t hook;                                // 回调函数
    const char * help;
} esh_cmd_reg_t;

static void esh_hook_null(char * mode, char * func, int num, char ** para);
static void esh_fkey_hook_default(uint8_t key_id);
static void esh_fkey_hook_null(uint8_t key_id);
static void esh_clear_buff_cmd(void);

// class -----------------------------------------------------------------------
typedef struct msh_tag
{
    uint32_t mode;
    uint32_t state;

    // 指令Buffer
    char buff_cmd[ESH_BUFF_CMD_SIZE + 1];
    uint32_t count_cmd;

    // 接收与发送Buffer
    uint8_t buff_recv[ESH_BUFF_RECV_SIZE];
    uint32_t count_recv;
    uint8_t buff_send[ESH_BUFF_SEND_SIZE];
    uint32_t count_send;

    // 指令注册表
    esh_cmd_reg_t reg_table[ESH_REG_TABLE_SIZE];  // 指令表
    uint32_t count_reg;

    esh_fkey_hook_t fkey_hook;
    esh_fkey_hook_t fkey_hook_default;
} msh_t;

msh_t esh;

// hooks -----------------------------------------------------------------------
static void hook_help(char * mode, char * func, int num, char ** para);

// default shell cmd table -----------------------------------------------------
#define HELP_INFORMATION "Msh, version 3.0.0.\n\r\
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
static const esh_cmd_reg_t esh_reg_table_default[] =
{
    { "help", Esh_Null, hook_help, HELP_INFORMATION },
};

// private data ----------------------------------------------------------------
static char * data_newline = "\n\r";
static eos_task_t task_esh;
static uint8_t stask_esh[ESH_TASK_STACK_SIZE];

// private function ------------------------------------------------------------
static bool esh_is_cmd_char(uint8_t cmd_char);
static void esh_clear_buff_cmd(void);
static bool esh_is_num_and_char(uint8_t cmd_char);
static void esh_task_funcntion(void *parameter);

// private function ------------------------------------------------------------
static void esh_cmd_parser(void);
static uint8_t esh_get_key_id(void);

// API -------------------------------------------------------------------------
void esh_init(void)
{
    esh.mode = EshMode_Log;
    esh.state = EshState_Unready;
    
    esh.count_cmd = 0;

    // 对各个Buffer进行初始化。
    for (uint32_t i = 0; i < ESH_BUFF_CMD_SIZE + 1; i ++)
    {
        esh.buff_cmd[i] = 0;
    }
    for (uint32_t i = 0; i < ESH_BUFF_RECV_SIZE; i ++)
    {
        esh.buff_recv[i] = 0;
    }
    for (uint32_t i = 0; i < ESH_BUFF_SEND_SIZE; i ++)
    {
        esh.buff_send[i] = 0;
    }
    esh.count_cmd = 0;
    esh.count_recv = 0;
    esh.count_send = 0;

    esh_port_init();

    // 特殊功能键回调函数
    esh.fkey_hook = esh_fkey_hook_null;
    esh.fkey_hook_default = esh_fkey_hook_null;
    
    /* Start the shell task. */
    eos_task_start(&task_esh, "task_esh",
                   esh_task_funcntion, 1, stask_esh, ESH_TASK_STACK_SIZE);
}

void esh_start(void)
{
    if (esh.state != EshState_Unready)
    {
        return;
    }

    esh.state = EshState_Log;
    const char * str_start = "\033[0;37m\n\rMsh started!\n\r";
    esh_port_send((void *)str_start, strlen(str_start));
}

bool esh_ready(void)
{
    return esh.state == EshState_Unready ? false : true;
}

void esh_mode(uint32_t mode)
{
    EOS_ASSERT(mode <= EshMode_FileRecv);

    // TODO
}

void esh_cmd_reg(const char * cmd, uint8_t shortcut, esh_hook_t hook, const char * help)
{
    EOS_ASSERT(shortcut >= Esh_Ctrl_Start && shortcut < Esh_Max);

    // 检查cmd的重复和shortcut的重复
    for (uint32_t i = 0; i < esh.count_cmd; i ++)
    {
        if (strcmp(cmd, esh.reg_table[i].name) == 0)
        {
            EOS_ASSERT(false);
        }
        if (esh.reg_table[i].shortcut == shortcut && shortcut != Esh_Null)
        {
            EOS_ASSERT(false);
        }
    }

    uint32_t m = esh.count_cmd;
    esh.reg_table[m].name = cmd;
    esh.reg_table[m].shortcut = shortcut;
    esh.reg_table[m].hook = hook;
    esh.reg_table[m].help = help;
    esh.count_cmd ++;
}

void esh_set_fkey_hook(esh_fkey_hook_t fkey_hook)
{
    esh.fkey_hook = fkey_hook;
}

void esh_log(const char * log)
{
    /* Unlock the elog mutex. */
    eos_mutex_take(ESH_MUTEX);
    
    if (esh.state != EshState_Log)
    {
        goto __EXIT;
    }

    esh_port_send((void *)log, strlen(log));

__EXIT:
    /* Unlock the elog mutex. */
    eos_mutex_release(ESH_MUTEX);
}

static void esh_task_funcntion(void *parameter)
{
    (void)parameter;
    
    esh_start();
    
    while (1) {
        uint64_t time_ms = eos_time();
        
        switch (esh.state)
        {
        case EshState_Unready:
            // 接收字符，但并不响应
            esh_port_recv(esh.buff_recv, ESH_BUFF_RECV_SIZE);
            esh.count_recv = 0;
            break;

        case EshState_Act: {
            // 接收计算
            uint32_t count_recv = esh_port_recv(&esh.buff_recv[esh.count_recv],
                                                  (ESH_BUFF_RECV_SIZE - esh.count_recv));
            esh.count_recv += count_recv;
            if (esh.count_recv == 0)
            {
                break;
            }
            // 获取KeyId
            uint8_t key_id = esh_get_key_id();
            // ESC 不理会
            if (key_id == Esh_Esc)
            {
                break;
            }
            // 特殊功能键，不理会
            if (key_id >= Esh_F1 && key_id <= Esh_PageDown)
            {
                break;
            }
            // 组合功能键
            if (key_id >= Esh_Ctrl_Start && key_id < Esh_Max)
            {
                // 在指令注册表中，寻找对应的指令
                bool exist = false;
                for (uint32_t i = 0; i < esh.count_reg; i ++)
                {
                    if (esh.reg_table[i].shortcut == key_id)
                    {
                        // TODO 解析指令

                        exist = true;
                        break;
                    }
                }
                if (exist == true)
                {
                    break;
                }
            }
            // 普通指令键
            if (esh_is_cmd_char(key_id))
            {
                // 写入指令Buff
                esh.buff_cmd[esh.count_cmd ++] = key_id;
                esh.buff_cmd[esh.count_cmd] = 0;
                // 回显
                esh_port_send(&key_id, 1);
                break;
            }
            // 普通功能键，回车键
            if (key_id == Esh_Enter)
            {
                // 解析指令
                esh_cmd_parser();
                break;
            }
            // 普通功能键，退格键
            if (key_id == Esh_Backspace)
            {
                if (esh.count_cmd <= 0)
                    break;
                esh.count_cmd --;
                esh.buff_cmd[esh.count_cmd] = 0;
                // 删除字符
                esh_port_send("\b \b", 3);
                break;
            }
            // 如果是其他字符，抛弃
            esh.count_recv -= count_recv;
            break;
        }

        case EshState_Log: {
            // 接收计算
            uint32_t count_recv = esh_port_recv(&esh.buff_recv[esh.count_recv],
                                                (ESH_BUFF_RECV_SIZE - esh.count_recv));
            esh.count_recv += count_recv;
            if (esh.count_recv == 0)
            {
                break;
            }
            // 获取KeyId
            uint8_t key_id = esh_get_key_id();
            // ESC，退出LOG模式
            if (key_id == Esh_Esc)
            {
                esh.state = EshState_Act;
                break;
            }
            // 特殊功能键
            if (key_id >= Esh_F1 && key_id <= Esh_PageDown)
            {
                esh.fkey_hook(key_id);
                break;
            }
            // 其他键，不予理会
            break;
        }

        default:
            break;
        }
    }
}

// private function ------------------------------------------------------------
static bool esh_is_cmd_char(uint8_t cmd_char)
{
    bool ret = false;

    if (cmd_char >= '0' && cmd_char <= '9')
    {
        ret = true;
    }
    else if (cmd_char >= 'a' && cmd_char <= 'z')
    {
        ret = true;
    }
    else if (cmd_char >= 'A' && cmd_char <= 'Z')
    {
        ret = true;
    }
    else if (cmd_char == ' ' || cmd_char == '-' || cmd_char == '_')
    {
        ret = true;
    }
    
    return ret;
}

static bool esh_is_num_and_char(uint8_t cmd_char)
{
    bool ret = false;

    if (cmd_char >= '0' && cmd_char <= '9')
    {
        ret = true;
    }
    else if (cmd_char >= 'a' && cmd_char <= 'z')
    {
        ret = true;
    }
    else if (cmd_char >= 'A' && cmd_char <= 'Z')
    {
        ret = true;
    }
    
    return ret;
}

static void esh_pop_buff_recv(uint8_t count)
{
    esh.count_recv -= count;
    for (uint32_t i = 0; i < esh.count_recv; i ++)
    {
        esh.buff_recv[i] = esh.buff_recv[i + count];
    }
}

static uint8_t esh_get_key_id(void)
{
    if (esh.count_recv == 0)
    {
        return Esh_Null;
    }
    
    // 检查是否特殊功能键
    if (esh.count_recv >= 3 && esh.buff_recv[0] == 0x1B)
    {
        for (uint32_t i = 0; i < sizeof(fkey_value_table) / sizeof(esh_key_info_t); i ++)
        {
            uint8_t length = fkey_value_table[i].length;
            uint8_t length_count = 0;
            for (uint32_t j = 0; j < fkey_value_table[i].length; j ++)
            {
                if (fkey_value_table[i].value[j] == esh.buff_recv[j])
                    length_count ++;
                else
                    break;
            }
            if (length == length_count)
            {
                esh_pop_buff_recv(length_count);
                return (Esh_Null + i);
            }
        }
    }

    // 检查指令码
    if (esh_is_cmd_char(esh.buff_recv[0]) == true)
    {
        esh_pop_buff_recv(1);
        return esh.buff_recv[0];
    }
    // 检查是否组合键
    for (uint32_t i = 0; i < sizeof(zkey_value_table) / sizeof(esh_key_info_t); i ++)
        if (zkey_value_table[i].value[0] == esh.buff_recv[0])
        {
            esh_pop_buff_recv(1);
            return (Esh_Ctrl_Start + i);
        }
    // 检查是否普通功能键
    for (uint32_t i = 0; i < sizeof(nkey_value_table) / sizeof(esh_key_info_t); i ++)
        if (nkey_value_table[i].value[0] == esh.buff_recv[0])
        {
            esh_pop_buff_recv(1);
            return (Esh_Backspace + i);
        }
    
    return Esh_Null;
}

static void esh_cmd_parser(void)
{
    // 如果没有指令码
    if (esh.count_cmd == 0)
    {
        esh_port_send((void *)data_newline, 2);
        return;
    }

    esh.buff_cmd[esh.count_cmd + 1] = 0;

    // 整理指令Buffer
    char cmd[esh.count_cmd + 1];
    uint32_t count_cmd = esh.count_cmd + 1;
    for (uint32_t i = 0; i < esh.count_cmd + 1; i ++)
        cmd[i] = 0;
    // 拷贝指令，并将所有的空格变为0.
    for (uint32_t i = 0; i < esh.count_cmd; i ++)
    {
        cmd[i] = esh.buff_cmd[i];
        if (cmd[i] == ' ')
            cmd[i] = 0;
    }

    #define ESH_CMD_MODE_SIZE                   16
    #define ESH_CMD_PARA_SIZE                   4

    // 解析指令的各个元素
    char * name = 0, * function = 0, * para[ESH_CMD_PARA_SIZE];
    char mode[ESH_CMD_MODE_SIZE];
    for (uint32_t i = 0; i < ESH_CMD_PARA_SIZE; i ++)
        para[i] = 0;
    for (uint32_t i = 0; i < ESH_CMD_MODE_SIZE; i ++)
        mode[i] = 0;
    uint32_t count_mode = 0;
    uint32_t count_para = 0;
    // 指令名称 例如mdb
    name = &cmd[0];
    // function --help --version之类
    for (uint32_t i = strlen(name); i < count_cmd - 3; i ++)
    {
        if (cmd[i] == 0 && cmd[i + 1] == '-' && cmd[i + 2] == '-' && cmd[i + 3] != '-')
        {
            function = &cmd[i + 1];
            goto SHELL_CMD_PARSER;
        }
    }
    // 模式 -rf -m -a之类
    for (uint32_t i = strlen(name); i < count_cmd - 1; i ++)
    {
        if (cmd[i] == 0 && cmd[i + 1] == '-')
        {
            uint32_t m = 0;
            while (cmd[i + 2 + m] != 0)
                mode[count_mode ++] = cmd[i + 2 + m ++];
            if (count_mode >= ESH_CMD_MODE_SIZE)
            {
                const char * info = "Wrong Cmd. Too many -mode.\n\r>> ";
                esh_port_send((void *)info, strlen(info));
                esh_clear_buff_cmd();
                return;
            }
        }
    }
    // 参数
    for (uint32_t i = strlen(name); i < count_cmd - 1; i ++)
    {
        if (cmd[i] == 0 && esh_is_num_and_char(cmd[i + 1]) == true)
        {
            para[count_para] = &cmd[i + 1];
            if (count_para >= ESH_CMD_PARA_SIZE)
            {
                const char * info = "Wrong Cmd. Too many para.\n\r>> ";
                esh_port_send((void *)info, strlen(info));
                esh_clear_buff_cmd();
                return;
            }
            
            count_para ++;
        }
    }
    
    // 解析
    uint16_t cmd_num = sizeof(esh_reg_table_default) / sizeof(esh_cmd_reg_t);

SHELL_CMD_PARSER:
    // 在指令注册列表中的指令
    for (uint32_t m = 0; m < esh.count_reg; m ++)
    {
        if (strcmp(name, esh.reg_table[m].name) != 0)
        {
            continue;
        }
        // 一个新行
        esh_port_send((void *)data_newline, 2);
        
        esh_port_send((void *)data_newline, 2);
        esh.reg_table[m].hook(mode, function, count_para, para);
        // clear all cmd char
        esh_clear_buff_cmd();
        return;
    }

    // 错误指令
    // a new line
    esh_port_send((void *)data_newline, 2);
    
    const char * wrong_cmd = "wrong cmd!";
    esh_port_send((void *)wrong_cmd, strlen(wrong_cmd));
    // clear all cmd char
    esh_clear_buff_cmd();
    // a new line
    esh_port_send(data_newline, 2);
}

static void esh_clear_buff_cmd(void)
{
    // clear all cmd char
    for (uint16_t i = 0; i < ESH_BUFF_CMD_SIZE; i ++)
    {
        esh.buff_cmd[i] = 0;
    }
    esh.count_cmd = 0;
}

static void esh_hook_null(char * mode, char * func, int num, char ** para)
{
    (void)mode;
    (void)func;
    (void)num;
    (void)para;
}

static void esh_fkey_hook_null(uint8_t key_id)
{
    (void)key_id;
}

static void esh_fkey_hook_default(uint8_t key_id)
{
    (void)key_id;
}

// hook ------------------------------------------------------------------------
static void hook_help(char * mode, char * func, int num, char ** para)
{
    const char * string;
    uint8_t newline[2] = {0x0D, 0x0A};
    string = "Shell help:\n\r";
    esh_port_send((void *)string, strlen(string));
    
    uint32_t cmd_num = sizeof(esh_reg_table_default) / sizeof(esh_cmd_reg_t);
    for (uint32_t i = 0; i < cmd_num; i ++)
    {
        esh_port_send("  + ", 4);
        esh_port_send((void *)esh_reg_table_default[i].help,
                        strlen(esh_reg_table_default[i].help));
        esh_port_send(newline, 2);
    }
    for (uint32_t i = 0; i < esh.count_cmd; i ++)
    {
        esh_port_send((void *)esh.reg_table[i].help,
                        strlen(esh.reg_table[i].help));
        esh_port_send(newline, 2);
    }
}
