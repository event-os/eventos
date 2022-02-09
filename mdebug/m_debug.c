
// include ---------------------------------------------------------------------
#include "m_debug.h"
#include "stdio.h"
#include "stdlib.h"
#include "stdbool.h"
#include <stdarg.h>
#include <string.h>
#include "m_assert.h"

M_MODULE_NAME("MDebug")

// data struct -----------------------------------------------------------------
typedef struct m_log_time_tag {
    int hour;
    int minute;
    int second;
    int ms;
} m_log_time_t;

typedef struct m_debug_tag {
    uint8_t buff[MDBG_BUFFER_SIZE];
    int count;
    bool enable;
    bool enable_flush;
    uint64_t time_ms_bkp;
    uint32_t time_ms_period;
    int level;

    const char * mod_name[MOD_ENABLE_SIZE];
    m_log_time_t log_time;
} m_debug_t;

static m_debug_t mdbg;

// static function -------------------------------------------------------------
static void m_dbg_time(uint64_t time_ms, m_log_time_t * log_time);
static bool m_dbg_mod_valid(const char * module);

// api -------------------------------------------------------------------------
void m_dbg_init(void)
{
    mdbg.count = 0;
    mdbg.enable = false;
    mdbg.enable_flush = false;
    mdbg.time_ms_bkp = 0;
    mdbg.time_ms_period = 0;
    mdbg.level = MLogLevel_Error;
    for (int i = 0; i < MOD_ENABLE_SIZE; i ++)
        mdbg.mod_name[i] = (void *)0;
}

void m_dbg_level(int level)
{
    mdbg.level = level;
}

void m_dbg_flush_enable(bool enable)
{
    mdbg.enable_flush = enable;
}

void m_dbg_enable(bool enable)
{
    mdbg.enable = enable;
}

void m_dbg_mod_enable(bool enable, const char * module)
{
    if (enable == true) {
        // 首先检查重复
        for (int i = 0; i < MOD_ENABLE_SIZE; i ++) {
            if (mdbg.mod_name[i] == 0)
                continue;
            if (strcmp(module, mdbg.mod_name[i]) == 0)
                return;
        }
        // 没有重复，检查是否有空位置
        for (int i = 0; i < MOD_ENABLE_SIZE; i ++) {
            if (mdbg.mod_name[i] == 0) {
                mdbg.mod_name[i] = module;
                return;
            }
        }
        // 没有空位置
        M_ASSERT(false);
    }
    else {
        // 检查存在
        for (int i = 0; i < MOD_ENABLE_SIZE; i ++) {
            if (strcmp(module, mdbg.mod_name[i]) == 0) {
                mdbg.mod_name[i] = 0;
                return;
            }
        }
    }
}

void m_printf(const char * s_format, ...)
{
    if (mdbg.level > MLogLevel_Print)
        return;
    va_list param_list;
    va_start(param_list, s_format);
    int i = vsnprintf((mdbg.buff + mdbg.count), 
                      (MDBG_BUFFER_SIZE - 1 - mdbg.count),
                      s_format, param_list);
    va_end(param_list);
    mdbg.count += i;

    if (mdbg.enable_flush == true)
        m_flush();
}

void m_print(const char * s_format, ...)
{
    if (mdbg.level > MLogLevel_Print)
        return;
    va_list param_list;
    va_start(param_list, s_format);
    int i = vsnprintf((mdbg.buff + mdbg.count),
                      (MDBG_BUFFER_SIZE - 3 - mdbg.count),
                      s_format, param_list);
    va_end(param_list);
    mdbg.count += i;
    *(mdbg.buff + mdbg.count ++) = '\n';
    *(mdbg.buff + mdbg.count ++) = '\r';

    if (mdbg.enable_flush == true)
        m_flush();
}

void m_info(const char * s_format, ...)
{
    if (mdbg.level > MLogLevel_Info)
        return;
    // check full
    const char * str_head = "\033[1;32m";
    const char * str_tail = "\033[0m\n\r";
    if ((mdbg.count + 29) >= MDBG_BUFFER_SIZE)
        return;
    // time 格式为[01-20-45-123]
    m_log_time_t time;
    m_dbg_time(port_sys_time_ms(), &time);
    char ctime[16];
    sprintf(ctime,
            "[%02d-%02d-%02d-%03d] ",
            time.hour, time.minute, time.second, time.ms);
    // 时间按照基础颜色显示
    for (int i = 0; i < 15; i ++)
        mdbg.buff[mdbg.count + i] = ctime[i];
    mdbg.count += 15;
    // head
    for (int i = 0; i < strlen(str_head); i ++)
        mdbg.buff[mdbg.count + i] = str_head[i];
    mdbg.count += 7;
    // info
    va_list param_list;
    va_start(param_list, s_format);
    int i = vsnprintf((mdbg.buff + mdbg.count),
                      (MDBG_BUFFER_SIZE - 7 - mdbg.count),
                      s_format, param_list);
    va_end(param_list);
    mdbg.count += i;
    // tail
    for (int i = 0; i < 6; i ++)
        mdbg.buff[mdbg.count + i] = str_tail[i];
    mdbg.count += 6;

    if (mdbg.enable_flush == true)
        m_flush();
}

void m_warn(const char * s_format, ...)
{
    if (mdbg.level > MLogLevel_Warn)
        return;
    // check full
    const char * str_head = "\033[1;33m";
    const char * str_tail = "\033[0m\n\r";
    if ((mdbg.count + 29) >= MDBG_BUFFER_SIZE)
        return;
    // time [01-20-45-123]
    m_log_time_t time;
    m_dbg_time(port_sys_time_ms(), &time);
    char ctime[16];
    sprintf(ctime,
            "[%02d-%02d-%02d-%03d] ",
            time.hour, time.minute, time.second, time.ms);
    // 时间按照基础颜色显示
    for (int i = 0; i < 15; i ++)
        mdbg.buff[mdbg.count + i] = ctime[i];
    mdbg.count += 15;
    // head
    for (int i = 0; i < strlen(str_head); i ++)
        mdbg.buff[mdbg.count + i] = str_head[i];
    mdbg.count += 7;
    // warn
    va_list param_list;
    va_start(param_list, s_format);
    int i = vsnprintf((mdbg.buff + mdbg.count),
                      (MDBG_BUFFER_SIZE - 7 - mdbg.count),
                      s_format, param_list);
    va_end(param_list);
    mdbg.count += i;
    // tail
    for (int i = 0; i < 6; i ++)
        mdbg.buff[mdbg.count + i] = str_tail[i];
    mdbg.count += 6;

    if (mdbg.enable_flush == true)
        m_flush();
}

void m_error(const char * s_format, ...)
{
    // check full
    const char * str_head = "\033[1;31m";
    const char * str_tail = "\033[0m\n\r";
    if ((mdbg.count + 29) >= MDBG_BUFFER_SIZE)
        return;
    // time 格式为[01-20-45-123]
    m_log_time_t time;
    m_dbg_time(port_sys_time_ms(), &time);
    char ctime[16];
    sprintf(ctime,
            "[%02d-%02d-%02d-%03d] ",
            time.hour, time.minute, time.second, time.ms);
    // 时间按照基础颜色显示
    for (int i = 0; i < 15; i ++)
        mdbg.buff[mdbg.count + i] = ctime[i];
    mdbg.count += 15;
    // head
    for (int i = 0; i < strlen(str_head); i ++)
        mdbg.buff[mdbg.count + i] = str_head[i];
    mdbg.count += 7;
    // warn
    va_list param_list;
    va_start(param_list, s_format);
    int i = vsnprintf((mdbg.buff + mdbg.count),
                      (MDBG_BUFFER_SIZE - 7 - mdbg.count),
                      s_format, param_list);
    va_end(param_list);
    mdbg.count += i;
    // tail
    for (int i = 0; i < 6; i ++)
        mdbg.buff[mdbg.count + i] = str_tail[i];
    mdbg.count += 6;

    if (mdbg.enable_flush == true)
        m_flush();
}

void m_flush(void)
{
    mdbg.buff[mdbg.count] = 0;
    port_dbg_out(mdbg.buff);
    mdbg.count = 0;
    mdbg.time_ms_bkp = port_sys_time_ms();
}

void m_print_poll(void)
{
    // log
    if (mdbg.enable == true) {
        if (mdbg.count == 0)
            mdbg.time_ms_bkp = port_sys_time_ms();
        else if (mdbg.count >= (int)(MDBG_BUFFER_SIZE * 80 / 100)) {
            mdbg.buff[mdbg.count] = 0;
            port_dbg_out(mdbg.buff);
            mdbg.count = 0;
            mdbg.time_ms_bkp = port_sys_time_ms();
        }
        else if ((port_sys_time_ms() - mdbg.time_ms_bkp) >= mdbg.time_ms_period) {
            mdbg.buff[mdbg.count] = 0;
            port_dbg_out(mdbg.buff);
            mdbg.count = 0;
            mdbg.time_ms_bkp = port_sys_time_ms();
        }
    }
    else
        mdbg.time_ms_bkp = port_sys_time_ms();
}

void m_assert(const char * module, const char * name, int id)
{
    if (name == 0)
        m_error("Assert: module %s, line %d.\n", module, id);
    else
        m_error("Assert: module %s, name: %s, line %d.\n", module, name, id);
    m_flush();
    port_after_assert();
}

void s_assert(const char * module, const char * name, int id)
{
    for (int i = 0; i < MDBG_BUFFER_SIZE; i ++)
        mdbg.buff[i] = 0;
    if (name == 0)
        sprintf((char *)mdbg.buff, "Module: %s, Id: %d.\n", module, id);
    else
        sprintf((char *)mdbg.buff, "Module: %s, Name: %s, Id: %d.\n", module, name, id);
    port_sassert_out((char *)mdbg.buff);
    port_after_assert();
}

void m_dbg_evt(const char * module, const char * state, const char * evt)
{
    if (m_dbg_mod_valid(module) == false)
        return;
    
    // [Time] [Module] E: State, Evt.
    m_log_time_t time;
    m_dbg_time(port_sys_time_ms(), &time);
    m_print("[%02d-%02d-%02d-%03d] [%s] %s, %s.",
            time.hour, time.minute, time.second, time.ms,
            module, state, evt);
}

// static function -------------------------------------------------------------
static bool m_dbg_mod_valid(const char * module)
{
    for (int i = 0; i < MOD_ENABLE_SIZE; i ++)
        if (strcmp(module, mdbg.mod_name[i]) == 0)
            return true;

    return false;
}

static void m_dbg_time(uint64_t time_ms, m_log_time_t * log_time)
{
    log_time->ms = (int)(time_ms % 1000);
    log_time->second = (time_ms / 1000) % 60;
    log_time->minute = (time_ms / 60000) % 60;
    log_time->hour = (time_ms / 3600000) % 24;
}
