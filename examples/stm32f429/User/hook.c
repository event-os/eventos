#include "eventos.h"

uint8_t flag_pub = 0;
uint32_t count_idle = 0;
void eos_hook_idle(void)
{
    count_idle ++;
    
    if (flag_pub != 0) {
        flag_pub = 0;
        eos_event_publish("Event_Time_1000ms");
    }
}

void eos_hook_start(void)
{

}

void eos_hook_stop(void)
{

}
