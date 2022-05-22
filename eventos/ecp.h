#ifndef __ECP_H
#define __ECP_H

#include <stdint.h>

// ECP的buffer满的策略
// 0 - 忽略
// 1 - 阻塞式等待
// 2 - 任务延时式等待
#define ECP_BUFF_FULL_STRATEGY                      1

enum ecp_return
{
    EcpRet_OK                       = 0,
    EcpRet_BufferFull               = -1,
};

// MCU
void ecp_init(void *buff_tx, uint32_t size_tx, void *buff_rx, uint32_t size_rx);
int32_t ecp_write(uint8_t type, void *data, uint32_t size);
int32_t ecp_read(uint8_t type, void *data, uint32_t size);

#endif
