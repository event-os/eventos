
#include "ecp.h"
#include "ecp_def.h"

#ifdef EOS_MCU
#define ECP_ASSERT(test)                ((void)0)
#else
#include <assert.h>
#define ECP_ASSERT                      assert
#endif

ecp_cb_t ecb;
static ecp_msg_t msg;
static uint32_t msg_tail = ECP_MSG_TAIL;

static uint8_t seqno_global = 0;

void ecp_init(void *buff_tx, uint32_t size_tx, void *buff_rx, uint32_t size_rx)
{
    ecb.magic = 0xdeadbeef;
    // ^*+ecp->block@$#
    ecb.label[0] = '^';
    ecb.label[1] = '*';
    ecb.label[2] = '+';
    ecb.label[3] = 'e';
    ecb.label[4] = 'c';
    ecb.label[5] = 'p';
    ecb.label[6] = '-';
    ecb.label[7] = '>';
    ecb.label[8] = 'b';
    ecb.label[9] = 'l';
    ecb.label[10] = 'o';
    ecb.label[11] = 'c';
    ecb.label[12] = 'k';
    ecb.label[13] = '@';
    ecb.label[14] = '$';
    ecb.label[15] = 0;

    ecb.buff_rx.buff = buff_rx;
    ecb.buff_rx.capacity = size_rx;

    ecb.buff_tx.buff = buff_tx;
    ecb.buff_tx.capacity = size_tx;
}

// TODO State变为STOP后并没有起到作用。
uint32_t loop_count = 0;
int32_t ecp_write(uint8_t type, void *data, uint32_t size)
{
    ECP_ASSERT(type <= EcpMsgType_Log);
    
    uint32_t size_total = sizeof(ecp_msg_t) + size + 4;
    ECP_ASSERT(size_total <= (ecb.buff_tx.capacity / 2));

    int32_t ret = Ecp_OK;

    uint32_t size_free;
    loop_count = 0;

    // xor & checksum
    uint8_t xor = ((uint8_t *)data)[0];
    uint8_t checksum = ((uint8_t *)data)[0];
    for (uint32_t i = 1; i < size; i ++)
    {
        xor ^= ((uint8_t *)data)[i];
        checksum += ((uint8_t *)data)[i];
    }
    
    // msg
    msg.head = ECP_MSG_HEAD;
    msg.type = type;
    msg.seqno = seqno_global;
    seqno_global ++;
    if (seqno_global == ECP_SEQNO_INVALID)
    {
        seqno_global = 0;
    }
    msg.length = size;
    msg.xor = xor;
    msg.checksum = checksum;

    uint32_t head = ecb.buff_tx.head;
    uint32_t capacity = ecb.buff_tx.capacity;
    // TODO Add Task mutex locking here.
    do
    {
        uint32_t tail = ecb.buff_tx.tail;
        if (head >= tail)
        {
            size_free = capacity - (head - tail) - 1;
        }
        else
        {
            size_free = tail - head - 1;
        }
        loop_count ++;
        if (loop_count > ECP_LOOP_COUNT_MAX)
        {
            ret = EcpErr_NotEnoughFreeMemory;
            goto EXIT;
        }
    } while (size_free < size_total);

    for (uint32_t i = 0; i < sizeof(ecp_msg_t); i ++)
    {
        ecb.buff_tx.buff[head] = ((uint8_t *)&msg)[i];
        head = (head + 1) % capacity;
    }
    for (uint32_t i = 0; i < size; i ++)
    {
        ecb.buff_tx.buff[head] = ((uint8_t *)data)[i];
        head = (head + 1) % capacity;
    }
    for (uint32_t i = 0; i < sizeof(msg_tail); i ++)
    {
        ecb.buff_tx.buff[head] = ((uint8_t *)&msg_tail)[i];
        head = (head + 1) % capacity;
    }
    ecb.buff_tx.head = head;

    // TODO Mutex locking here.

    ret = (int32_t)size;

EXIT:
    return ret;
}

int32_t ecp_read(uint8_t type, void *data, uint32_t size)
{
    (void)type;
    (void)data;
    (void)size;

    return 0;
}
