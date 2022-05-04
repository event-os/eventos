
#include "esh.h"
#include <stdint.h>
#include <string.h>

#define ELOG_BUFFER_SIZE                        1024

typedef struct elog_block
{
    char tag[16];
    uint32_t count;
    uint32_t size;
    char buffer[ELOG_BUFFER_SIZE];
} elog_block_t;

elog_block_t elog_block;

void esh_port_init(void)
{
    memset(&elog_block, 0, sizeof(elog_block_t));
    strcat(elog_block.tag, "__elog_");
    strcat(elog_block.tag, "block");
    elog_block.size = ELOG_BUFFER_SIZE;
}

void esh_port_send(void * data, uint32_t size)
{
    uint32_t length = size;
    if (elog_block.count + length >= elog_block.size)
    {
        return;
    }
    
    memcpy(&elog_block.buffer[elog_block.count], data, size);
    elog_block.count += size;
    elog_block.buffer[elog_block.count] = 0;
}

uint8_t esh_port_recv(void * data, uint8_t size)
{
    return 0;
}
