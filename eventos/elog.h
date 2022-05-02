
#ifndef __ELOG_H__
#define __ELOG_H__

#ifdef __cplusplus
extern "C" {
#endif

// 格式 ---------------------------------------------------------------------
// [Time] [Module] E: State, Evt.
// [Time] [Module] L: Log.

// include ---------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>

// config ----------------------------------------------------------------------
#define ELOG_DEV_MAX                            8
#define ELOG_BUFF_LOG_SIZE                      1024
#define ELOG_SIZE_LOG                           256
#define ELOG_MAX_OBJECTS                        32
#define ELOG_TAG_MAX_LENGTH                     16
#define ELOG_LINE_FEED                          0   // 0: \n, 1: \r\n

// level -----------------------------------------------------------------------
enum {
    eLogLevel_Debug = 0,
    eLogLevel_Info,
    eLogLevel_Warn,
    eLogLevel_Error,

    eLogLevel_Disable,
};

enum
{
    eLogMode_BlackList = 0,
    eLogMode_WhiteList,
};

typedef struct elog_device
{
    struct elog_device *next;

    const char *name;
    uint8_t level;
    bool enable;

    void (* out)(char *data);
    void (* flush)(void);
    bool (* ready)(void);
} elog_device_t;

// api -------------------------------------------------------------------------
// basic
void elog_init(void);
void elog_start(void);
void elog_stop(void);
void elog_set_level(uint8_t level);

// device
void elog_device_register(elog_device_t *device);
void elog_device_attribute(const char *name, bool enable, uint8_t level);

// level
void elog_device_level(const char *dev, uint32_t level);
void elog_tag_level(const char *tag, uint32_t level);

// filter
void elog_tag_mode(uint8_t mode);
void elog_tag_clear_all(void);
void elog_tag_attribute(const char *tag, bool enable, uint8_t level);

// 下面的函数是线程安全的，其他都不是。
// print
void elog_printf(const char *s_format, ...);
void elog_debug(const char *tag, const char *s_format, ...);
void elog_info(const char *tag, const char *s_format, ...);
void elog_warn(const char *tag, const char *s_format, ...);
void elog_error(const char *tag, const char *s_format, ...);
void elog_assert(const char *tag, const char *name, uint32_t id);
void elog_assert_info(const char *tag, const char *s_format, ...);
void elog_flush(void);

#ifdef __cplusplus
}
#endif

#endif
