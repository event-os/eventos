
#ifndef EOS_TEST_H__
#define EOS_TEST_H__

#include <stdbool.h>
#include "eventos.h"

#if (EOS_USE_SM_MODE != 0)
/* actors for test ---------------------------------------------------------- */
// fsm led -----------------------------------------------
typedef struct fsm_tag {
    eos_sm_t super;
    uint32_t count;
    uint32_t state;
    uint32_t data_size;
} fsm_t;

void fsm_init(  fsm_t * const me,
                const char *name,
                uint8_t priority,
                void const *stack, uint32_t size);
uint32_t fsm_state(fsm_t * const me);
uint32_t fsm_event_count(fsm_t * const me);
void fsm_reset_event_count(fsm_t * const me);

#endif

// reactor -----------------------------------------------
typedef struct reactor_tag {
    eos_reactor_t super;
    bool status;
    uint32_t value;
    int count_test;
    int count_tr;
    int data_size;
} reactor_t;

void reactor_init(  reactor_t * const me,
                    const char *name,
                    uint8_t priority,
                    void const *stack, uint32_t size);
int reactor_e_test_count(reactor_t * const me);
int reactor_e_tr_count(reactor_t * const me);
uint32_t reactor_get_value(reactor_t * const me);

/* tool --------------------------------------------------------------------- */
void set_time_ms(uint32_t time_ms);

/* test function ------------------------------------------------------------ */
void eos_test_etimer(void);
void eos_test_event(void);
void eos_test_heap(void);
void eos_test_fsm(void);
void eos_test_hsm(void);
void eos_test_reactor(void);
void eos_test_sub(void);

#endif
