#include "eventos.h"
#include "event_def.h"
#include "eos_led.h"

static eos_u32_t eos_sub_table[Event_Max];

int main(void)
{
    eventos_init(eos_sub_table);

    eos_led_init();

    eventos_run();

    return 0;
}