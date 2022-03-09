
/*
 * EventOS Nano
 * Copyright (c) 2021, EventOS Team, <event-os@outlook.com>
 *
 * SPDX-License-Identifier: MIT
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the 'Software'), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS 
 * OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 * IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.event-os.cn
 * https://github.com/event-os/eventos-nano
 * https://gitee.com/event-os/eventos-nano
 * 
 * Change Logs:
 * Date           Author        Notes
 * 2022-02-20     DogMing       V0.0.2
 */

#ifndef EVENTOS_DEF_H__
#define EVENTOS_DEF_H__

#include "eventos_config.h"

/* basic data type ---------------------------------------------------------- */
typedef unsigned int                    eos_u32_t;
typedef signed int                      eos_s32_t;
typedef unsigned short                  eos_u16_t;
typedef signed short                    eos_s16_t;
typedef unsigned char                   eos_u8_t;
typedef signed char                     eos_s8_t;

typedef enum eos_bool {
    EOS_False = 0,
    EOS_True = !EOS_False,
} eos_bool_t;

#define EOS_NULL                        ((void *)0)

#define EOS_U32_MAX                     0xffffffff
#define EOS_U32_MIN                     0

#define EOS_U16_MAX                     0xffff
#define EOS_U16_MIN                     0

#define EOS_HEAP_MAX                    0x7fff

#if (EOS_MCU_TYPE == 8)
typedef eos_u8_t                        eos_mcu_t;
#elif (EOS_MCU_TYPE == 16)
typedef eos_u16_t                       eos_mcu_t;
#else
typedef eos_u32_t                       eos_mcu_t;
#endif

#if (EOS_MCU_TYPE == 8)
typedef eos_u8_t                        eos_sub_t;
#elif (EOS_MCU_TYPE == 16)
typedef eos_u16_t                       eos_sub_t;
#else
typedef eos_u32_t                       eos_sub_t;
#endif

#if (EOS_TEST_PLATFORM == 32)
typedef eos_u32_t                       eos_pointer_t;
#else
#include <stdint.h>
typedef uint64_t                        eos_pointer_t;
#endif

#endif
