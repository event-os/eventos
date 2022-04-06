#include "eventos.h"

uint8_t flag_pub = 0;
void eos_hook_idle(void)
{
    if (flag_pub != 0) {
        flag_pub = 0;
        eos_event_pub_topic("Event_Time_1000ms");
    }
}

void eos_hook_start(void)
{

}

void eos_hook_stop(void)
{

}
