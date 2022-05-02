
#include "shell/shell.h"
#include "stdlib.h"
#include "devf.h"
#include "serial/dev_serial.h"
#include "dev_def.h"
#include "evt_def.h"
#include "mlog/mlog.h"
#include "mqttclient.h"

static device_t * serial_debug;
inline void * shell_port_malloc(int size)
{
    return malloc(size);
}

void shell_port_io_init(void)
{

}

void shell_port_set_recv_evt(int evt_id)
{

}
