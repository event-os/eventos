#include "eventos.h"
#include "event_def.h"
#include "eos_led.h"

static eos_u32_t eos_sub_table[Event_Max];
static eos_u8_t eos_heap_memory[1024];

int main(void)
{
    eventos_init();
    eos_sub_init(eos_sub_table);
    eos_event_pool_init(eos_heap_memory, 1024);

    eos_led_init();

    eventos_run();

    return 0;
}