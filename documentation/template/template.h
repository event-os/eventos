
/*
 * EventOS Template V0.1
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
 * 2021-11-23     XiaoMing      the first version
 */

#ifndef EOS_TEMPLATE_H_
#define EOS_TEMPLATE_H_

// include path ----------------------------------------------------------------
#include "eos_def.h"

#ifdef __cplusplus
extern "C" {
#endif

// data structure --------------------------------------------------------------
typedef struct eos_structure {
    eos_u32_t u32;
    eos_s32_t s32;
    eos_u16_t u16;
    eos_s16_t s16;
} eos_structure_t;

typedef struct eos_enum {
    EosEnum_Tip0 = 0,
    EosEnum_Tip1,
    EosEnum_Tip2,

    EosEnum_Maxd
} eos_enum_t;

typedef eos_s32_t (* eos_hook_t)(eos_obj_t *me, void *data, eos_u32_t size);

// API -------------------------------------------------------------------------
eos_s32_t eos_module_init(void);
void eos_module_poll(eos_time_t time);
void eos_module_end(void);

#ifdef __cplusplus
}
#endif

#endif
