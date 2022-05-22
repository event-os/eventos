
#include "esh.h"
#include <stdint.h>
#include <string.h>
#include "ecp.h"
#include "ecp_def.h"

#define ELOG_BUFFER_TX_SIZE                        102400
#define ELOG_BUFFER_RX_SIZE                        4

static uint8_t buff_tx[ELOG_BUFFER_TX_SIZE];
static uint8_t buff_rx[ELOG_BUFFER_RX_SIZE];

void esh_port_init(void)
{
    ecp_init(buff_tx, ELOG_BUFFER_TX_SIZE,
             buff_rx, ELOG_BUFFER_RX_SIZE);
}

void esh_port_send(void * data, uint32_t size)
{
    ecp_write(EcpMsgType_Log, data, size);
}

uint8_t esh_port_recv(void * data, uint8_t size)
{
    return 0;
}
