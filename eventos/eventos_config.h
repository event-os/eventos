
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

#ifndef EVENTOS_CONFIG_H__
#define EVENTOS_CONFIG_H__

// <<< Use Configuration Wizard in Context Menu >>>

/* EventOS Nano General Configuration --------------------------------------- */

// <h> EventOS Nano's basic configuration
//   <o>  The MCU type: 8, 16 or 32 bits
#define EOS_MCU_TYPE                            32

//   <o>  The maximum number of actors: 1 - 32
#define EOS_MAX_ACTORS                          32

//   <o>  The platform type: 32 ort 64 bits.
#define EOS_TEST_PLATFORM                       32

//   <o>  The time of system tick.
#define EOS_TICK_MS                             1


/* Assert Configuration ----------------------------------------------------- */
//   <o>  use ASSERT or not
#define EOS_USE_ASSERT                          1

// </h>

/* State Machine Function Configuration ------------------------------------- */
// <h> EventOS Nano's state-machine configuration
//   <o>  use state-machine mode (1 or 0)
#define EOS_USE_SM_MODE                         1

//   <o>  use HSM mode (1 or 0)
#define EOS_USE_HSM_MODE                        1
#if (EOS_USE_SM_MODE != 0 && EOS_USE_HSM_MODE != 0)

//   <o>  use hsm nest depth (2 - 4)
#define EOS_MAX_HSM_NEST_DEPTH                  4
#endif

/* Publish & Subscribe Configuration ---------------------------------------- */
#define EOS_USE_PUB_SUB                         1
// </h>

/* Time Event Configuration ------------------------------------------------- */
#define EOS_USE_TIME_EVENT                      1
#if (EOS_USE_TIME_EVENT != 0)
    #define EOS_MAX_TIME_EVENT                  4           // 时间事件的数量
#endif

/* Event's Data Configuration ----------------------------------------------- */
#define EOS_USE_EVENT_DATA                      1
#define EOS_SIZE_HEAP                           12800       // 设定堆大小

/* Event Bridge Configuration ----------------------------------------------- */
#define EOS_USE_EVENT_BRIDGE                    0

/* Error -------------------------------------------------------------------- */
#if ((EOS_MCU_TYPE != 8) && (EOS_MCU_TYPE != 16) && (EOS_MCU_TYPE != 32))
#error The MCU type must be 8-bit, 16-bit or 32-bit !
#endif

#if ((EOS_TEST_PLATFORM != 32) && (EOS_TEST_PLATFORM != 64))
#error The test paltform must be 32-bit or 64-bit !
#endif

#if (EOS_MAX_ACTOR > EOS_MCU_TYPE || EOS_MAX_ACTORS <= 0)
#error The maximum number of actors must be 1 ~ EOS_MCU_TYPE !
#endif

#if (EOS_USE_SM_MODE != 0)
    #if (EOS_USE_HSM_MODE != 0)
        #if (EOS_MAX_HSM_NEST_DEPTH > 4 || EOS_MAX_HSM_NEST_DEPTH < 2)
            #error The maximum nested depth of hsm must be 2 ~ 4 !
        #endif
    #endif
#endif

#if (EOS_USE_TIME_EVENT != 0 && EOS_MAX_TIME_EVENT >= 256)
    #error The number of time events must be less than 256 !
#endif

#if (EOS_USE_EVENT_DATA != 0)
    #if (EOS_USE_HEAP != 0 && (EOS_SIZE_HEAP < 128 || EOS_SIZE_HEAP > EOS_HEAP_MAX))
        #error The heap size must be 128 ~ 32767 (32KB) if the function is enabled !
    #endif
#endif

#endif
