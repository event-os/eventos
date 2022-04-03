/* include ------------------------------------------------------------------ */
#include "eos_led.h"
#include "eventos.h"
#include "event_def.h"
#include <stdio.h>

#if (EOS_USE_SM_MODE != 0)
/* data structure ----------------------------------------------------------- */
typedef struct eos_sm_led_tag {
    eos_sm_t super;

    eos_u8_t status;
} eos_sm_led_t;

eos_sm_led_t sm_led;

/* static state function ---------------------------------------------------- */
static eos_ret_t state_init(eos_sm_led_t * const me, eos_event_t const * const e);
static eos_ret_t state_on(eos_sm_led_t * const me, eos_event_t const * const e);
static eos_ret_t state_off(eos_sm_led_t * const me, eos_event_t const * const e);

/* api ---------------------------------------------------- */
eos_u64_t stack_sm[32];
void eos_sm_led_init(void)
{
    eos_sm_init(&sm_led.super, 1, stack_sm, sizeof(stack_sm));
    eos_sm_start(&sm_led.super, EOS_STATE_CAST(state_init));

    sm_led.status = 0;
}

/* static state function ---------------------------------------------------- */
static eos_ret_t state_init(eos_sm_led_t * const me, eos_event_t const * const e)
{
#if (EOS_USE_PUB_SUB != 0)
    EOS_EVENT_SUB(Event_Time_500ms);
#endif
    eos_event_pub_period(Event_Time_500ms, 500);

    return EOS_TRAN(state_off);
}

static eos_ret_t state_on(eos_sm_led_t * const me, eos_event_t const * const e)
{
    switch (e->topic) {
        case Event_Enter:
            me->status = 1;
            return EOS_Ret_Handled;
        
        case Event_Exit:
            return EOS_Ret_Handled;

        case Event_Time_500ms:
            return EOS_TRAN(state_off);

        default:
            return EOS_SUPER(eos_state_top);
    }
}

static eos_ret_t state_off(eos_sm_led_t * const me, eos_event_t const * const e)
{
    switch (e->topic) {
        case Event_Enter:
            me->status = 0;
            return EOS_Ret_Handled;
        
        case Event_Exit:
            return EOS_Ret_Handled;

        case Event_Time_500ms:
            return EOS_TRAN(state_on);

        default:
            return EOS_SUPER(eos_state_top);
    }
}
#endif
