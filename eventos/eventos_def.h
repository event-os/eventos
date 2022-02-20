#ifndef EVENTOS_DEF_H__
#define EVENTOS_DEF_H__

#include "eventos_config.h"

/* basic data type ---------------------------------------------------------- */
typedef unsigned int                    eos_u32_t;
typedef int                             eos_s32_t;
typedef unsigned short                  eos_u16_t;
typedef short                           eos_s16_t;
typedef unsigned char                   eos_u8_t;
typedef char                            eos_s8_t;

typedef enum eos_bool {
    EOS_False = 0,
    EOS_True = !EOS_False,
} eos_bool_t;

#define EOS_U32_MAX                     0xffffffff
#define EOS_U32_MIN                     0

#if (EOS_MCU_TYPE == 8)
typedef eos_u8_t                        eos_mcu_t;
#elif (EOS_MCU_TYPE == 16)
typedef eos_u16_t                       eos_mcu_t;
#else
typedef eos_u32_t                       eos_mcu_t;
#endif

#endif
