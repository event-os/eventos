
#include "esh.h"
#include <stdint.h>
#include "RTT/SEGGER_RTT.h"

void esh_port_init(void)
{
    SEGGER_RTT_Init();
}

void esh_port_send(void * data, uint32_t size)
{
    SEGGER_RTT_Write(0, data, size);
}

uint8_t esh_port_recv(void * data, uint8_t size)
{
    uint8_t *buffer = (uint8_t *)data;
    uint8_t count = 0;

    for (uint8_t i = 0; i < size; i ++)
    {
        int32_t key_id = SEGGER_RTT_GetKey();
        if (key_id < 0)
        {
            break;
        }
        
        buffer[count ++] = (uint8_t)key_id;
    }

    return count;
}
