
// include ---------------------------------------------------------------------
#include "elog.h"
#include "eventos.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

EOS_TAG("elog")

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------------------------------------
Define
----------------------------------------------------------------------------- */
#define ELOG_MUTEX                      "mutex_elog"
#define ELOG_MUTEX_EN                   1

// data struct -----------------------------------------------------------------
typedef struct elog_time_tag
{
    uint32_t hour               : 5;
    uint32_t minute             : 6;
    uint32_t rsv                : 5;
    uint32_t second             : 6;
    uint32_t ms                 : 10;
} elog_time_t;

typedef struct elog_object
{
    char tag[ELOG_TAG_MAX_LENGTH];
    uint8_t level;
} elog_object_t;

typedef struct elog
{
    char buff_assert[ELOG_SIZE_LOG];
    uint32_t count_assert;
    bool enable;
    uint32_t level;
    uint32_t color;
    uint8_t mode;
    elog_time_t time;
} elog_t;

typedef struct elog_hash
{
    elog_object_t obj[ELOG_MAX_OBJECTS];
    uint16_t prime_max;
} elog_hash_t;

// data ------------------------------------------------------------------------
static elog_hash_t hash;
static elog_device_t *dev_list;
elog_t elog;

extern volatile int8_t eos_interrupt_nest;

static const char *string_color_log[] = {
    "\033[0m", "\033[1;32m", "\033[1;33m", "\033[1;31m"
};
static const char *string_level_log[] = {
    "D", "I", "W", "E"
};

// static function -------------------------------------------------------------
static void __elog_time(uint64_t time_ms, elog_time_t *log_time);
static uint32_t __hash_time33(const char *string);
static uint16_t __hash_insert(const char *string);
static uint16_t __hash_get_index(const char *string);

// api -------------------------------------------------------------------------
void elog_init(void)
{
    // Initialize the hash table.
    /* Find the maximum prime in the range of ELOG_MAX_OBJECTS. */
    for (int32_t i = ELOG_MAX_OBJECTS; i > 0; i --)
    {
        bool is_prime = true;
        for (uint32_t j = 2; j < ELOG_MAX_OBJECTS; j ++)
        {
            if (i <= j)
            {
                break;
            }
            if ((i % j) == 0)
            {
                is_prime = false;
                break;
            }
        }
        if (is_prime == true)
        {
            hash.prime_max = i;
            break;
        }
    }
    for (uint32_t i = 0; i < ELOG_MAX_OBJECTS; i ++)
    {
        hash.obj[i].tag[0] = 0;
        hash.obj[i].level = eLogLevel_Error;
    }

    elog.count_assert = 0;
    elog.enable = false;
    elog.level = eLogLevel_Error;
    elog.mode = eLogMode_BlackList;

    for (int32_t i = 0; i < ELOG_SIZE_LOG; i ++)
        elog.buff_assert[i] = 0;
    elog.count_assert = 0;
}

void elog_device_register(elog_device_t *device)
{
    /* The function is not permitted to use in interrupt function. */
    EOS_ASSERT(eos_interrupt_nest == 0);

#if (ELOG_MUTEX_EN != 0)
    /* Lock the elog mutex. */
    eos_mutex_take(ELOG_MUTEX);
#endif
    
    /* It's not permitted that log device is regitstered after the log module
       starts. */
    EOS_ASSERT(elog.enable == false);

    /* Check the log device has not same name with the former devices. */
    elog_device_t *next = dev_list;
    while (next != NULL)
    {
        EOS_ASSERT(strcmp(next->name, device->name) != 0);
        next = next->next;
    }

    /* Add the log device to the list. */
    device->next = dev_list;
    dev_list = device;

#if (ELOG_MUTEX_EN != 0)
    /* Unlock the elog mutex. */
    eos_mutex_release(ELOG_MUTEX);
#endif
}

void elog_device_attribute(const char * name, bool enable, uint8_t level)
{
    /* Lock the elog mutex. */
    eos_mutex_take(ELOG_MUTEX);

    /* It's not permitted that log device is regitstered after the log module
       starts. */
    EOS_ASSERT(dev_list != NULL);

    /* Check the log device has not same name with the former devices. */
    elog_device_t *next = dev_list;
    while (next != NULL)
    {
        if (strcmp(next->name, name) == 0)
        {
            next->enable = enable;
            next->level = level;

            /* Unlock the elog mutex. */
            eos_mutex_release(ELOG_MUTEX);

            return;
        }
        else
        {
            next = next->next;
        }
    }

    /* Log device not found. */
    EOS_ASSERT(0);
}

void elog_start(void)
{
    elog.enable = true;
}

void elog_stop(void)
{
    elog.enable = false;
}

void elog_set_level(uint8_t level)
{
    elog.level = level;
}

void elog_dev_level(const char *dev, uint32_t level)
{
    (void)dev;
    (void)level;
}

void elog_tag_level(const char *tag, uint32_t level)
{
    (void)tag;
    (void)level;
}

static char buff[ELOG_SIZE_LOG];
static char ctime[48];
static elog_time_t time;
void __elog_print(const char *tag, uint8_t level, bool lf_en, const char * s_format, va_list *param_list)
{
    /* It's not permitted that log device is regitstered after the log module
       starts. */
    EOS_ASSERT(dev_list != NULL);

    int32_t len;
    int32_t count;
    elog_device_t *next;
    int32_t len_head;
    bool valid;

#if (ELOG_MUTEX_EN != 0)
    /* Lock the elog mutex. */
    eos_mutex_take(ELOG_MUTEX);
#endif
    
    if (elog.enable == false)
    {
        goto __ELOG_PRINT_EXIT;
    }
    if (elog.level > level)
    {
        goto __ELOG_PRINT_EXIT;
    }
    
    if (lf_en == true)
    {
        valid = false;
        
        if (__hash_get_index(tag) != ELOG_MAX_OBJECTS)
        {
            valid = (elog.mode == eLogMode_BlackList) ? false : true;
        }
    }

    memset(buff, 0, ELOG_SIZE_LOG);
    if (elog.color != level)
    {
        elog.color = level;
        /* Output the buffer data if device is ready. */
        next = dev_list;
        while (next != NULL)
        {
            if (next->ready())
            {
                next->out((char *)string_color_log[level]);
            }
            next = next->next;
        }
    }

    __elog_time(eos_time(), &time);
    len_head = sprintf(ctime,"[%02d:%02d:%02d %03d] %s (%s) ",
                        time.hour, time.minute, time.second, time.ms,
                        string_level_log[level], tag);

    if (lf_en == true)
    {
        strcat(buff, ctime);
        len = strlen(buff) + len_head;
    }
    len = strlen(buff);
    count = vsnprintf(&buff[len], (ELOG_SIZE_LOG - 3 - len), s_format, *param_list);
    
    if (lf_en == true)
    {
        len += count;
        if (ELOG_LINE_FEED == 0)
        {
            buff[len ++] = '\n';
        }
        else
        {
            buff[len ++] = '\r';
            buff[len ++] = '\n';
        }
    }
    
    next = dev_list;
    while (next != NULL)
    {
        if (next->ready())
        {
            next->out(buff);
        }
        next = next->next;
    }

__ELOG_PRINT_EXIT:
#if (ELOG_MUTEX_EN != 0)
    eos_mutex_release(ELOG_MUTEX);      /* Unlock the elog mutex. */
#endif
}

void elog_printf(const char *s_format, ...)
{
    va_list param_list;
    va_start(param_list, s_format);
    __elog_print("__null", eLogLevel_Debug, false, s_format, &param_list);
    va_end(param_list);
}

void elog_debug(const char *tag, const char *s_format, ...)
{
    EOS_ASSERT(strlen(tag) < 16);

    va_list param_list;
    va_start(param_list, s_format);
    __elog_print(tag, eLogLevel_Debug, true, s_format, &param_list);
    va_end(param_list);
}

void elog_info(const char *tag, const char *s_format, ...)
{
    EOS_ASSERT(strlen(tag) < 16);
    
    va_list param_list;
    va_start(param_list, s_format);
    __elog_print(tag, eLogLevel_Info, true, s_format, &param_list);
    va_end(param_list);
}

void elog_warn(const char *tag, const char *s_format, ...)
{
    EOS_ASSERT(strlen(tag) < 16);
    
    va_list param_list;
    va_start(param_list, s_format);
    __elog_print(tag, eLogLevel_Warn, true, s_format, &param_list);
    va_end(param_list);
}

void elog_error(const char *tag, const char *s_format, ...)
{
    EOS_ASSERT(strlen(tag) < 16);

    va_list param_list;
    va_start(param_list, s_format);
    __elog_print(tag, eLogLevel_Error, true, s_format, &param_list);
    va_end(param_list);
}

void elog_flush(void)
{
#if (ELOG_MUTEX_EN != 0)
    /* Lock the elog mutex. */
    eos_mutex_take(ELOG_MUTEX);
#endif
    
    /* It's not permitted that log device is regitstered after the log module
       starts. */
    EOS_ASSERT(dev_list != NULL);

    /* Check the log device has not same name with the former devices. */
    elog_device_t *next = dev_list;
    while (next != NULL)
    {
        next->flush();
        next = next->next;
    }

#if (ELOG_MUTEX_EN != 0)
    /* Unlock the elog mutex. */
    eos_mutex_release(ELOG_MUTEX);
#endif
}

uint32_t eos_error_id = 0;
void elog_assert(const char *tag, const char *name, uint32_t id)
{
    eos_error_id = id;
    
    char buff[ELOG_SIZE_LOG];
    memset(buff, 0, ELOG_SIZE_LOG);
    strcat(buff, string_color_log[eLogLevel_Error]);
    int32_t len_color = strlen(buff);

    // For example: Assert! EventOS, HookExcute, 420.
    if (name != 0)
    {
        sprintf(&buff[len_color], "Assert! %s, %s, %d.\n\r", tag, name, id);
    }
    
    // For example: Assert! EventOS, 420.
    else
    {
        sprintf(&buff[len_color], "Assert! %s, %d.\n\r", tag, id);
    }

#if (ELOG_MUTEX_EN != 0)
    /* Unlock the elog mutex. */
    eos_mutex_take(ELOG_MUTEX);
#endif
    
    /* Check the log device has not same name with the former devices. */
    elog_device_t *next = dev_list;
    while (next != NULL)
    {
        next->out(buff);
        next->flush();
        next = next->next;
    }
    
#if (ELOG_MUTEX_EN != 0)
    /* Unlock the elog mutex. */
    eos_mutex_release(ELOG_MUTEX);
#endif
    
    eos_interrupt_disable();
    while (1)
    {
    }
}

void elog_assert_info(const char *tag, const char *s_format, ...)
{
    char buff[ELOG_SIZE_LOG];
    memset(buff, 0, ELOG_SIZE_LOG);
    
    int32_t len = sprintf(buff, "%sAssert! (%s) ",
                          string_color_log[eLogLevel_Error],
                          tag);
    
    va_list param_list;
    va_start(param_list, s_format);
    vsnprintf(&buff[len], ELOG_SIZE_LOG - len, s_format, param_list);
    va_end(param_list);
    
#if (ELOG_MUTEX_EN != 0)
    /* Unlock the elog mutex. */
    eos_mutex_take(ELOG_MUTEX);
#endif
    
    /* Check the log device has not same name with the former devices. */
    elog_device_t *next = dev_list;
    while (next != NULL)
    {
        next->out(buff);
        next->flush();
        next = next->next;
    }
    
#if (ELOG_MUTEX_EN != 0)
    /* Unlock the elog mutex. */
    eos_mutex_release(ELOG_MUTEX);
#endif
    
    eos_interrupt_disable();
    while (1)
    {
    }
}

// static function -------------------------------------------------------------
static void __elog_time(uint64_t time_ms, elog_time_t *log_time)
{
    log_time->ms = (uint32_t)(time_ms % 1000);
    log_time->second = (time_ms / 1000) % 60;
    log_time->minute = (time_ms / 60000) % 60;
    log_time->hour = (time_ms / 3600000) % 24;
}

static uint32_t __hash_time33(const char *string)
{
    uint32_t hash = 5381;
    while (*string)
    {
        hash += (hash << 5) + (*string ++);
    }

    return (uint32_t)(hash & INT32_MAX);
}

static uint16_t __hash_insert(const char *string)
{
    uint16_t index = 0;

    /* Calculate the hash value of the string. */
    uint32_t hash_value = __hash_time33(string);
    uint16_t index_init = hash_value % hash.prime_max;

    for (uint16_t i = 0; i < (ELOG_MAX_OBJECTS / 2 + 1); i ++)
    {
        for (int8_t j = -1; j <= 1; j += 2)
        {
            index = index_init + i * j + 2 * (int16_t)ELOG_MAX_OBJECTS;
            index %= ELOG_MAX_OBJECTS;

            /* Find the empty object. */
            if (hash.obj[index].tag[0] == 0)
            {
                strcpy(hash.obj[index].tag, string);
                return index;
            }
            if (strcmp(hash.obj[index].tag, string) != 0)
            {
                continue;
            }
            
            return index;
        }
    }

    /* If this assert is trigged, you need to enlarge the hash table size. */
    EOS_ASSERT(0);
    
    return 0;
}

static uint16_t __hash_get_index(const char *string)
{
    uint16_t index = 0;

    /* Calculate the hash value of the string. */
    uint32_t hash_value = __hash_time33(string);
    uint16_t index_init = hash_value % hash.prime_max;

    for (uint16_t i = 0; i < (ELOG_MAX_OBJECTS / 2 + 1); i ++)
    {
        for (int8_t j = -1; j <= 1; j += 2)
        {
            index = index_init + i * j + 2 * (int16_t)ELOG_MAX_OBJECTS;
            index %= ELOG_MAX_OBJECTS;

            if (hash.obj[index].tag[0] != 0 &&
                strcmp(hash.obj[index].tag, string) == 0)
            {
                return index;
            }
        }
    }
    
    return ELOG_MAX_OBJECTS;
}

#ifdef __cplusplus
}
#endif
