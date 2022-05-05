
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
    uint32_t hour                       : 5;
    uint32_t minute                     : 6;
    uint32_t rsv                        : 5;
    uint32_t second                     : 6;
    uint32_t ms                         : 10;
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

static const char *string_color_log[] =
{
    "\033[0m", "\033[1;32m", "\033[1;33m", "\033[1;31m"
};
static const char *string_level_log[] =
{
    "D", "I", "W", "E"
};

// static function -------------------------------------------------------------
static void __elog_time(uint64_t time_ms, elog_time_t *log_time);
static uint32_t __hash_time33(const char *string);
static uint16_t __hash_insert(const char *string);
static uint16_t __hash_get_index(const char *string);
static int32_t eos_sprintf(char *buffer, const char * s_format, ...);

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
    {
        elog.buff_assert[i] = 0;
    }
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
static elog_time_t time;
void __elog_print(const char *tag, uint8_t level, bool lf_en, const char * s_format, va_list *param_list)
{
    /* It's not permitted that log device is regitstered after the log module
       starts. */
    EOS_ASSERT(dev_list != NULL);

    int32_t len;
    int32_t count;
    elog_device_t *next;
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

    memset(buff, 0, ELOG_SIZE_LOG);
    if (lf_en == true)
    {
        __elog_time(eos_time(), &time);
        len = sprintf(buff,"[%02d:%02d:%02d %03d] %s (%s) ",
                      time.hour, time.minute, time.second, time.ms,
                      string_level_log[level], tag);
    }
    count = vsnprintf(&buff[len], (ELOG_SIZE_LOG - 3 - len), s_format, *param_list);
    len += count;

    if (lf_en == true)
    {
#if (ELOG_LINE_FEED != 0)
        buff[len ++] = '\r';
#endif
        buff[len ++] = '\n';
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

#define FORMAT_FLAG_LEFT_JUSTIFY   (1u << 0)
#define FORMAT_FLAG_PAD_ZERO       (1u << 1)
#define FORMAT_FLAG_PRINT_SIGN     (1u << 2)
#define FORMAT_FLAG_ALTERNATE      (1u << 3)

typedef struct
{
    char * p_buff;
    uint32_t cnt;
} eos_buffer_t;

static void _print_unsigned(eos_buffer_t * p_buff,
                            uint32_t v, uint32_t base, uint32_t num_digits,
                            uint32_t field_width, uint32_t format_flags)
{
    static const char _av2c[16] =
    {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 
        'A', 'B', 'C', 'D', 'E', 'F'
    };
    uint32_t div;
    uint32_t digit;
    uint32_t number;
    uint32_t width;
    char c;

    number = v;
    digit = 1u;

    /* Get actual field width. */
    width = 1u;
    while (number >= base)
    {
        number = (number / base);
        width ++;
    }
    if (num_digits > width)
    {
        width = num_digits;
    }

    /* Print leading chars if necessary. */
    if ((format_flags & FORMAT_FLAG_LEFT_JUSTIFY) == 0u)
    {
        if (field_width != 0u)
        {
            if (((format_flags & FORMAT_FLAG_PAD_ZERO) == FORMAT_FLAG_PAD_ZERO) &&
                (num_digits == 0u))
            {
                c = '0';
            }
            else
            {
                c = ' ';
            }
            while ((field_width != 0u) && (width < field_width))
            {
                field_width--;
                *(p_buff->p_buff + p_buff->cnt ++) = c;
            }
        }
    }

    /*  Compute Digit.
        Loop until Digit has the value of the highest digit required.
        Example: If the output is 345 (Base 10), loop 2 times until Digit is 100. */
    while (1)
    {
        /*  User specified a min number of digits to print? => Make sure we loop
            at least that often, before checking anything else (> 1 check avoids
            problems with num_digits being signed / unsigned). */
        if (num_digits > 1u)
        {       
            num_digits--;
        }
        else
        {
            div = v / digit;
            /*  Is our divider big enough to extract the highest digit from value?
                => Done */
            if (div < base)
            {
                break;
            }
        }
        digit *= base;
    }

    /* Output digits. */
    do
    {
        div = v / digit;
        v -= div * digit;
        *(p_buff->p_buff + p_buff->cnt ++) = _av2c[div];
        digit /= base;
    } while (digit);

    /* Print trailing spaces if necessary. */
    if ((format_flags & FORMAT_FLAG_LEFT_JUSTIFY) == FORMAT_FLAG_LEFT_JUSTIFY)
    {
        if (field_width != 0u)
        {
            while ((field_width != 0u) && (width < field_width))
            {
                field_width--;
                *(p_buff->p_buff + p_buff->cnt ++) = ' ';
            }
        }
    }
}

static void _print_int( eos_buffer_t * p_buff,
                        int32_t v, uint32_t base, uint32_t num_digits,
                        uint32_t field_width, uint32_t format_flags)
{
    uint32_t width;
    int32_t number;

    number = (v < 0) ? -v : v;

    /* Get actual field width. */
    width = 1u;
    while (number >= (int)base)
    {
        number = (number / (int)base);
        width ++;
    }
    if (num_digits > width)
    {
        width = num_digits;
    }
    if ((field_width > 0u) && ((v < 0) ||
        ((format_flags & FORMAT_FLAG_PRINT_SIGN) == FORMAT_FLAG_PRINT_SIGN)))
    {
        field_width--;
    }

    /* Print leading spaces if necessary. */
    if ((((format_flags & FORMAT_FLAG_PAD_ZERO) == 0u) ||
        (num_digits != 0u)) && ((format_flags & FORMAT_FLAG_LEFT_JUSTIFY) == 0u))
    {
        if (field_width != 0u)
        {
            while ((field_width != 0u) && (width < field_width))
            {
                field_width --;
                *(p_buff->p_buff + p_buff->cnt ++) = ' ';
            }
        }
    }

    /* Print sign if necessary. */
    if (v < 0)
    {
        v = -v;
        *(p_buff->p_buff + p_buff->cnt ++) = '-';
    }
    else if ((format_flags & FORMAT_FLAG_PRINT_SIGN) == FORMAT_FLAG_PRINT_SIGN)
    {
        *(p_buff->p_buff + p_buff->cnt ++) = '+';
    }
    else
    {
    }

    /* Print leading zeros if necessary. */
    if (((format_flags & FORMAT_FLAG_PAD_ZERO) == FORMAT_FLAG_PAD_ZERO) &&
        ((format_flags & FORMAT_FLAG_LEFT_JUSTIFY) == 0u) && (num_digits == 0u))
    {
        if (field_width != 0u)
        {
            while ((field_width != 0u) && (width < field_width))
            {
                field_width --;
                *(p_buff->p_buff + p_buff->cnt ++) = '0';
            }
        }
    }

    /* Print number without sign. */
    _print_unsigned(p_buff, (uint32_t)v, base, num_digits, field_width, format_flags);
}

int32_t eos_vprintf(char *buff, const char * s_format, va_list * para_list)
{
    char c;
    eos_buffer_t buffer;
    int32_t v;
    uint32_t num_digits;
    uint32_t format_flags;
    uint32_t field_width;

    buffer.p_buff = buff;
    buffer.cnt = 0u;

    while (1)
    {
        c = *s_format;
        s_format ++;
        if (c == 0u)
        {
            break;
        }
        if (c == '%')
        {
            /* Filter out flags */
            format_flags = 0u;
            v = 1;
            do
            {
                c = *s_format;
                switch (c)
                {
                case '-': format_flags |= FORMAT_FLAG_LEFT_JUSTIFY; s_format++; break;
                case '0': format_flags |= FORMAT_FLAG_PAD_ZERO;     s_format++; break;
                case '+': format_flags |= FORMAT_FLAG_PRINT_SIGN;   s_format++; break;
                case '#': format_flags |= FORMAT_FLAG_ALTERNATE;    s_format++; break;
                default:  v = 0; break;
                }
            } while (v);

            /* filter out field with */
            field_width = 0u;
            do
            {
                c = *s_format;
                if ((c < '0') || (c > '9'))
                {
                    break;
                }
                s_format++;
                field_width = (field_width * 10u) + ((uint32_t)c - '0');
            } while (1);

            /* Filter out precision (number of digits to display). */
            num_digits = 0u;
            c = *s_format;
            if (c == '.')
            {
                s_format++;
                do
                {
                    c = *s_format;
                    if ((c < '0') || (c > '9'))
                    {
                        break;
                    }
                    s_format++;
                    num_digits = num_digits * 10u + ((uint32_t)c - '0');
                } while (1);
            }
            /* Filter out length modifier. */
            c = *s_format;
            do
            {
                if ((c == 'l') || (c == 'h'))
                {
                    s_format++;
                    c = *s_format;
                }
                else
                {
                    break;
                }
            } while (1);

            /* Handle specifiers. */
            switch (c)
            {
            case 'c':
            {
                char c0;
                v = va_arg(*para_list, int);
                c0 = (char)v;
                *(buffer.p_buff + buffer.cnt ++) = c0;
                break;
            }

            case 'd':
                v = va_arg(*para_list, int);
                _print_int(&buffer, v, 10u, num_digits, field_width, format_flags);
                break;

            case 'u':
                v = va_arg(*para_list, int);
                _print_unsigned(&buffer, (uint32_t)v, 10u, num_digits, field_width, format_flags);
                break;

            case 'x':
            case 'X':
                v = va_arg(*para_list, int);
                _print_unsigned(&buffer, (uint32_t)v, 16u, num_digits, field_width, format_flags);
                break;

            case 's':
            {
                const char * s = va_arg(*para_list, const char *);
                while (1)
                {
                    c = *s;
                    s ++;
                    if (c == '\0')
                    {
                        break;
                    }
                    *(buffer.p_buff + buffer.cnt ++) = c;
                }
            }
            break;

            case 'p':
                v = va_arg(*para_list, int);
                _print_unsigned(&buffer, (uint32_t)v, 16u, 8u, 8u, 0u);
                break;

            case '%':
                *(buffer.p_buff + buffer.cnt ++) = '%';
                break;

            default:
                break;
            }
            s_format ++;
        }
        else
        {
            *(buffer.p_buff + buffer.cnt ++) = c;
        }
    }

    *(buffer.p_buff + buffer.cnt) = 0;

    return buffer.cnt;
}

int eos_sprintf(char *buffer, const char * s_format, ...)
{
    int32_t r;
    va_list para_list;

    va_start(para_list, s_format);
    r = eos_vprintf(buffer, s_format, &para_list);
    va_end(para_list);

    return r;
}

#ifdef __cplusplus
}
#endif
