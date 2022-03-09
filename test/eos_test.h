
#ifndef EOS_TEST_H__
#define EOS_TEST_H__

#include <stdbool.h>
#include "eventos.h"

#if (EOS_USE_SM_MODE != 0)
/* actors for test ---------------------------------------------------------- */
// fsm led -----------------------------------------------
typedef struct fsm_tag {
    eos_sm_t super;
    eos_u32_t count;
    eos_u32_t state;
} fsm_t;

void fsm_init(fsm_t * const me, eos_u8_t priority, void const * const parameter);
eos_u32_t fsm_state(fsm_t * const me);
eos_u32_t fsm_event_count(fsm_t * const me);
void fsm_reset_event_count(fsm_t * const me);

#endif

// reactor -----------------------------------------------
typedef struct reactor_tag {
    eos_reactor_t super;
    bool status;
    int count_test;
    int count_tr;
} reactor_t;

void reactor_init(reactor_t * const me, eos_u8_t priority, void const * const parameter);
int reactor_e_test_count(reactor_t * const me);
int reactor_e_tr_count(reactor_t * const me);

/* tool --------------------------------------------------------------------- */
void set_time_ms(eos_u32_t time_ms);

/* test function ------------------------------------------------------------ */
void eos_test_etimer(void);
void eos_test_event(void);
void eos_test_heap(void);
void eos_test_fsm(void);
void eos_test_hsm(void);
void eos_test_reactor(void);
void eos_test_sub(void);

#endif
