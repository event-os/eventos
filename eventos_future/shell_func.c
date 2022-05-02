
// include ---------------------------------------------------------------------
#include "shell.h"
#include "stdlib.h"
#include "shell_config.h"
#include "mlog/mlog.h"

// shell的base功能 -------------------------------------------------------------
#include "base/dev_base.h"
#include "string.h"

M_TAG("ShellFunc")

// base [--help] [--version] cmd [<para>]
// 帮助信息和版本信息 --------------------------------------------------
static const char * base_help_info = "\
Usage: base [--help] [--version] cmd [<args>]\n\r\
Help:\n\r\
\n\r\
Function:\n\r\
    ma              set the vm value in the manual mode.\n\r\
    motor           run one motor. Format: [base motor 1 200].\n\r\
\n\r\
Special Function Key:\n\r\
    Up              move robot manually in the positive direction.\n\r\
    Down            move robot manually in the negative direction.\n\r\
    Left            turn robot to the left direction.\n\r\
    Right           turn robot to the right direction.\n\r\
    F1              stop robot.\n\r\
\n\r\
Common:\n\r\
    --help          print help information.\n\r\
    --version       print version information.\n\r";
static const char * version_information = "base version 0.1.0.\n\r";

// 特殊功能键回调 --------------------------------------------------
static int v_manual = 500;
static int w_manual = 30;
static void shell_fkey_hook_base_ma(uint8_t key_id)
{
    if (key_id == Key_Up) {
        v_manual = v_manual >= 0 ? v_manual : -v_manual;
        base_vw(v_manual, 0);
        M_INFO("Key_Up.");
    }
    else if (key_id == Key_Down) {
        v_manual = v_manual < 0 ? v_manual : -v_manual;
        base_vw(v_manual, 0);
        M_INFO("Key_Down.");
    }
    else if (key_id == Key_Right) {
        w_manual = w_manual >= 0 ? w_manual : -w_manual;
        base_vw(v_manual, w_manual);
        M_INFO("Key_Right.");
    }
    else if (key_id == Key_Left) {
        w_manual = w_manual < 0 ? w_manual : -w_manual;
        base_vw(v_manual, w_manual);
        M_INFO("Key_Left.");
    }
    else if (key_id == Key_F1) {
        base_vw(0, 0);
        M_INFO("Key_F1.");
    }
}

static void shell_hook_base(char * mode, char * func, int num, char ** para)
{
    if (func != 0) {
        if (strcmp("--help", func) == 0)
            shell_print("%s", base_help_info);
        else if (strcmp("--version", func) == 0)
            shell_print("%s", version_information);
        else
            shell_print("Unknown option: %s.\n\r", func);
        return;
    }

    // 如果cmd参数不存在，返回信息
    if (num == 0 || para[0] == 0) {
        shell_print("mdb: NOT complete cmd.");
        return;
    }

    if (strcmp("ma", para[0]) == 0) {
        shell_mode(ShellMode_Act);
        if (mode == 0)
            v_manual = 400;
        else if (mode[0] == '1')
            v_manual = 500;
        else if (mode[0] == '2')
            v_manual = 450;
        else if (mode[0] == '3')
            v_manual = 400;
        else if (mode[0] == '4')
            v_manual = 350;
        else if (mode[0] == '5')
            v_manual = 300;
        else
            shell_print("Unknown mode: -%s.", mode);
        // 刷新模式显示数据
        // for (int i = 0; i < sizeof(status_keys) / sizeof(const char *); i ++)
        //     shell_db_refresh(status_keys[i]);
        shell_set_fkey_hook(shell_fkey_hook_base_ma);
        shell_mode(ShellMode_Refresh);
        return;
    }

    shell_print("base: %s is not a mc commond. See 'base --help'.\n\r", para[0]);
}


// shell file function ---------------------------------------------------------
#if (SHELL_FILE_FUNC_EN != 0)

#include "ff.h"

// ls [--help] [--version] [<path>]
static const char * help_info_ls = "\
Usage: ls [--help] [--version]\n\r";
static const char * version_info_ls = "ym version 1.0.0.\n\r";
static void shell_hook_ls(char * mode, char * func, int num, char ** para)
{
    if (func != 0) {
        shell_print("Unknown option: --%s.", func);
        return;
    }

    FRESULT result;
    FATFS fs;
    DIR DirInf;  
    FILINFO FileInf;
    
    uint8_t tmpStr[20];
    
     // 挂载文件系统
    result = f_mount(0, &fs);
    if (result != FR_OK) {
        shell_print("f_mount fail. error: %d.\r\n", result);
        return;
    }

    // 打开根文件夹
    result = f_opendir(&DirInf, "/");
    if (result != FR_OK) {
        shell_print("f_opendir fail. error: %d.\r\n", result);
        return;
    }

    // 读取当前文件夹下的文件和目录
    shell_print("Name\t\tTyepe\t\tSize\r\n");
    for (int cnt = 0; ; cnt ++) {
        // 读取目录项，索引会自动下移。
        result = f_readdir(&DirInf,&FileInf);
        if (result != FR_OK || FileInf.fname[0] == 0)
            break;
        
        if (FileInf.fname[0] == '.')
            continue;
        
        shell_print("%s", FileInf.fname);
        if (strlen(FileInf.fname) < 8)        // 对齐
            shell_print("\t\t");
        else
            shell_print("\t");

        if (FileInf.fattrib == AM_DIR)
            shell_print("catalogue\t\t");
        else 
            shell_print("file\t\t");

        shell_print("%d\r\n", FileInf.fsize);
    }

    // 卸载文件系统
    f_mount(0, (void *)0);
}
#endif

#if (SHELL_FILE_FUNC_EN != 0)

#include "ff.h"

static const char * help_info_touch = "\
Usage: ls [--help] [--version]\n\r";
static const char * version_info_touch = "ym version 1.0.0.\n\r";
static void shell_hook_touch(char * mode, char * func, int num, char ** para)
{
    if (func != 0) {
        shell_print("Unknown option: --%s.", func);
        return;
    }

    FRESULT result;
    FATFS fs;
    DIR DirInf;  
    FILINFO FileInf;
    FIL file;
    uint32_t bw;
    
    uint8_t tmpStr[20];
    
     // 挂载文件系统
    result = f_mount(0, &fs);
    if (result != FR_OK) {
        shell_print("f_mount fail. error: %d.\r\n", result);
        return;
    }

    // 打开根文件夹
    result = f_opendir(&DirInf, "/");
    if (result != FR_OK) {
        shell_print("f_opendir fail. error: %d.\r\n", result);
        return;
    }

    shell_print("f_open oooooo.\r\n");
    result = f_open(&file, "ooooo.txt", FA_CREATE_ALWAYS | FA_WRITE);
    if (result != FR_OK) {
        shell_print("f_open fail. error: %d.\r\n", result);
        return;
    }
    
    result = f_write(&file, "FatFS Write Demo \r\n www.armfly.com \r\n", 34, &bw);
    if (result != FR_OK) {
        shell_print("f_write fail. error: %d.\r\n", result);
        return;
    }

    f_close(&file);
    f_mount(0, NULL);
}
#endif

// -----------------------------------------------------------------------------
void shell_port_default_func_reg(void)
{
    shell_cmd_reg("base", shell_hook_base, "base - help");
    
#if (SHELL_FILE_FUNC_EN != 0)
    shell_cmd_reg("ls", shell_hook_ls, "ls - help");
#endif
}
