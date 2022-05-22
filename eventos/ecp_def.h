#ifndef ECP_DEF_H
#define ECP_DEF_H

#include <stdint.h>

enum
{
    Ecp_OK = 0,
    EcpErr_NotEnoughFreeMemory              = -1,
};

enum
{
    EcpState_Init = 0,
    EcpState_Running,
    EcpState_Stop,
    EcpState_Stopped,
};

#define ECP_LOOP_COUNT_MAX                  (1000000)

typedef struct ecp_buff
{
    uint8_t *buff;
    uint32_t capacity;
    uint32_t head;
    uint32_t tail;
} ecp_buff_t;

typedef struct ecp_info
{
    uint32_t state          : 2;
    uint32_t rsv            : 30;
} ecp_info_t;

typedef struct ecp_cb
{
    // Host Read only
    uint32_t magic;
    uint8_t label[16];  // ^*+ecp->block@$
    ecp_info_t info;
    ecp_buff_t buff_tx;
    ecp_buff_t buff_rx;
} ecp_cb_t;

#define ECP_MSG_HEAD                        (0xAA55AA55)
#define ECP_MSG_TAIL                        (0x55AA55AA)

#define ECP_SEQNO_INVALID                   (0xff)

enum
{
    EcpMsgType_Log = 0,
    EcpMsgType_Scope,
    EcpMsgType_PRC,
};

#pragma pack(1)
typedef struct ecp_msg
{
    uint32_t head;
    uint8_t seqno;                          // Max 250
    uint8_t type;
    uint16_t length;                        // Max 62500
    uint8_t xor;                            // Verification
    uint8_t checksum;
} ecp_msg_t;
#pragma pack()

#endif
