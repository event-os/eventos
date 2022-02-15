#ifndef BSP_H
#define BSP_H

#include "stdint.h"

void bsp_init(void);
uint64_t bsp_get_time_ms(void);
void systick_enable(void);

#endif
