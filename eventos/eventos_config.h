
/*
 * EventOS
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
 * https://github.com/event-os/eventos
 * https://gitee.com/event-os/eventos
 * 
 * Change Logs:
 * Date           Author        Notes
 * 2022-02-20     DogMing       V0.0.2
 */

#ifndef EVENTOS_CONFIG_H__
#define EVENTOS_CONFIG_H__

// <<< Use Configuration Wizard in Context Menu >>>

/* EventOS General Configuration --------------------------------------- */

// <h> EventOS Nano's basic configuration
//   <o>  The maximum number of tasks: 1 - 64
#define EOS_MAX_TASKS                           32

//   <o>  The maximum number of objects: 16 - 65536
#define EOS_MAX_OBJECTS                         32
#define EOS_MAX_HASH_SEEK_TIMES                 5

//   <o>  The platform type: 32 ort 64 bits.
#define EOS_TEST_PLATFORM                       32

//   <o>  The time of system tick.
#define EOS_TICK_MS                             1

//    <o>  use stack usage function (0 or 1) <0-1>
#define EOS_USE_STACK_USAGE                     1

//    <o>  use cpu usage function (0 or 1) <0-1>
#define EOS_USE_CPU_USAGE                       1

/* Assert Configuration ----------------------------------------------------- */
//   <o>  use ASSERT or not (0 or 1) <0-1>
#define EOS_USE_ASSERT                          1

// </h>

/* State Machine Function Configuration ------------------------------------- */
// <h> EventOS Nano's state-machine configuration
//   <o>  use state-machine mode (0 or 1) <0-1>
#define EOS_USE_SM_MODE                         1

//   <o>  use HSM mode (0 or 1) <0-1>
#define EOS_USE_HSM_MODE                        1
#if (EOS_USE_SM_MODE != 0 && EOS_USE_HSM_MODE != 0)

//   <o>  use hsm nest depth (2 - 4) <2-4>
#define EOS_MAX_HSM_NEST_DEPTH                  4
#endif
// </h>

/* Publish & Subscribe Configuration ---------------------------------------- */
// <h> EventOS Nano's event configuration

//   <o>  use event pub-sub mode (0 or 1) <0-1>
#define EOS_USE_PUB_SUB                         1


/* Time Event Configuration ------------------------------------------------- */
//   <o>  use time event (0 or 1) <0-1>
#define EOS_USE_TIME_EVENT                      1
#if (EOS_USE_TIME_EVENT != 0)
    //   <o>  The maximum number of event timers (1 - 256)
    #define EOS_MAX_TIME_EVENT                  4           // 时间事件的数量
#endif

/* Event's Data Configuration ----------------------------------------------- */
//   <o>  use time event (0 ort 1) <0-1>
#define EOS_USE_EVENT_DATA                      1

//   <o>  The maximum size of event heap (128 - 32767) <128-32767>
#define EOS_SIZE_HEAP                           32000       // 设定堆大小

/* Event Bridge Configuration ----------------------------------------------- */
//   <o>  use event bridge (0 or 1) <0-1>
#define EOS_USE_EVENT_BRIDGE                    0
// </h>

/* Error -------------------------------------------------------------------- */
#if ((EOS_TEST_PLATFORM != 32) && (EOS_TEST_PLATFORM != 64))
#error The test paltform must be 32-bit or 64-bit !
#endif

#if (EOS_MAX_TASKS > 32 || EOS_MAX_TASKS <= 0)
#error The maximum number of tasks must be 1 ~ 32 !
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
