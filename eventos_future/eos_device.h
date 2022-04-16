
#ifndef EOS_DEVICE_H__
#define EOS_DEVICE_H__

#include <stdint.h>
#include <stdbool.h>

/* -----------------------------------------------------------------------------
TODO 实现。Device
----------------------------------------------------------------------------- */
typedef struct eos_device {
    const char *name;
} eos_device_t;

void eos_device_register(eos_device_t * const me, const char *name);
void eos_device_enable(eos_device_t * const me, bool status);
void eos_device_read(eos_device_t *const me, uint8_t pos, void *const buffer, uint32_t size);
void eos_device_write(eos_device_t *const me, uint8_t pos, void *const buffer, uint32_t size);
void eos_device_control(eos_device_t *const me, uint32_t cmd, void *paremeter);
void eos_device_poll(uint64_t time_us);

#endif
