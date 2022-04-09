/* include ------------------------------------------------------------------ */
#include "eos_led.h"
#include "eventos.h"
#include <stdio.h>

#if (EOS_USE_SM_MODE != 0)
/* data structure ----------------------------------------------------------- */
typedef struct eos_sm_led_tag {
    eos_sm_t super;

    uint8_t status;
} eos_sm_led_t;

eos_sm_led_t sm_led;

/* static state function ---------------------------------------------------- */
static eos_ret_t state_init(eos_sm_led_t * const me, eos_event_t const * const e);
static eos_ret_t state_on(eos_sm_led_t * const me, eos_event_t const * const e);
static eos_ret_t state_off(eos_sm_led_t * const me, eos_event_t const * const e);

/* api ---------------------------------------------------- */
uint8_t stack_sm[256];
void eos_sm_led_init(void)
{
    eos_sm_init(&sm_led.super, "sm_led", 1, stack_sm, sizeof(stack_sm));
    sm_led.status = 0;
    
    eos_sm_start(&sm_led.super, EOS_STATE_CAST(state_init));
}

/* static state function ---------------------------------------------------- */
static eos_ret_t state_init(eos_sm_led_t * const me, eos_event_t const * const e)
{
#if (EOS_USE_PUB_SUB != 0)
    eos_event_sub("Event_Time_500ms");
#endif
    eos_event_pub_period("Event_Time_500ms", 500);

    return EOS_TRAN(state_off);
}

static eos_ret_t state_on(eos_sm_led_t * const me, eos_event_t const * const e)
{
    if (eos_event_topic(e, "Event_Enter")) {
        me->status = 1;
        return EOS_Ret_Handled;
    }
    
    if (eos_event_topic(e, "Event_Exit")) {
        return EOS_Ret_Handled;
    }
    
    if (eos_event_topic(e, "Event_Time_500ms")) {
        return EOS_TRAN(state_off);
    }
    
    return EOS_SUPER(eos_state_top);
}

static eos_ret_t state_off(eos_sm_led_t * const me, eos_event_t const * const e)
{
    if (eos_event_topic(e, "Event_Enter")) {
        me->status = 0;
        return EOS_Ret_Handled;
    }
    
    if (eos_event_topic(e, "Event_Exit")) {
        return EOS_Ret_Handled;
    }
    
    if (eos_event_topic(e, "Event_Time_500ms")) {
        return EOS_TRAN(state_on);
    }
    
    return EOS_SUPER(eos_state_top);
}
#endif
