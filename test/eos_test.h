
#ifndef EOS_TEST_H__
#define EOS_TEST_H__

#include <stdbool.h>
#include "eventos.h"

/* actors for test ---------------------------------------------------------- */
// fsm led -----------------------------------------------
typedef struct fsm_tag {
    eos_sm_t super;
    bool status;
    int count;
} fsm_t;

void fsm_init(fsm_t * const me, eos_u8_t priority, void *queue, eos_u32_t queue_size);
int fsm_get_evt_count(fsm_t * const me);
void fsm_reset_evt_count(fsm_t * const me);

// hsm ---------------------------------------------------
typedef struct hsm_tag {
    eos_sm_t super;
    bool status;
    int count;
} hsm_t;

void hsm_init(hsm_t * const me, eos_u8_t priority, void *queue, eos_u32_t queue_size);
int hsm_get_status(hsm_t * const me);

// reactor -----------------------------------------------
typedef struct reactor_tag {
    eos_sm_t super;
    bool status;
    int count;
} reactor_t;

/* test function ------------------------------------------------------------ */
void eos_test_etimer(void);
void eos_test_event(void);
void eos_test_heap(void);
void eos_test_fsm(void);
void eos_test_hsm(void);
void eos_test_reactor(void);
void eos_test_sub(void);

#endif
