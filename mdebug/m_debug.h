
#ifndef __M_DEBUG_H__
#define __M_DEBUG_H__

// 格式 ---------------------------------------------------------------------
// [Time] [Module] E: State, Evt.
// [Time] [Module] L: Log.

// include ---------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>

// config ----------------------------------------------------------------------
#define MDBG_BUFFER_SIZE                        1024
#define MOD_ENABLE_SIZE                         128

// level -----------------------------------------------------------------------
enum {
    MLogLevel_Print = 0,
    MLogLevel_Info,
    MLogLevel_Warn,
    MLogLevel_Error,
};

// api -------------------------------------------------------------------------
void m_dbg_init(void);
void m_dbg_level(int level);
void m_dbg_flush_enable(bool enable);
void m_dbg_enable(bool enable);
void m_dbg_mod_enable(bool enable, const char * module);
void m_dbg_poll(void);

void m_printf(const char * s_format, ...);
void m_print(const char * s_format, ...);
void m_info(const char * s_format, ...);
void m_warn(const char * s_format, ...);
void m_error(const char * s_format, ...);

void m_dbg_evt(const char * module, const char * state, const char * evt_name);
void m_flush(void);
void m_assert(const char * module, const char * name, int id);
void s_assert(const char * module, const char * name, int id);

#define M_ERROR                         m_error
#define M_INFO                          m_info
#define M_WARN                          m_warn
#define M_DEBUG                         m_print

// port ------------------------------------------------------------------------
void port_dbg_init(void);
void port_dbg_out(char * p_char);
uint64_t port_sys_time_ms(void);
void * port_dbg_malloc(int size);
void port_after_assert(void);
void port_sassert_out(char * p_char);

#endif
