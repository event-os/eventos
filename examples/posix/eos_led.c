#include "eos_led.h"
#include "eventos.h"
#include "event_def.h"
#include <stdio.h>

typedef struct eos_led_tag {
    eos_sm_t super;

    eos_bool_t status;
} eos_led_t;

static eos_led_t led;

// state -----------------------------------------------------------------------
static eos_ret_t state_init(eos_led_t * const me, eos_event_t const * const e);
static eos_ret_t state_on(eos_led_t * const me, eos_event_t const * const e);
static eos_ret_t state_off(eos_led_t * const me, eos_event_t const * const e);

void eos_led_init(void)
{
    static eos_u32_t queue[32];
    eos_sm_init(&led.super, 1, queue, 32, 0, 0);
    eos_sm_start(&led.super, EOS_STATE_CAST(state_init));

    led.status = 0;
}

// state -----------------------------------------------------------------------
static eos_ret_t state_init(eos_led_t * const me, eos_event_t const * const e)
{
    EOS_EVENT_SUB(Event_Time_500ms);
    eos_event_pub_period(Event_Time_500ms, 500);

    return EOS_TRAN(state_off);
}

static eos_ret_t state_on(eos_led_t * const me, eos_event_t const * const e)
{
    switch (e->topic) {
        case Event_Enter:
            printf("State On!\n");
            me->status = 1;
            return EOS_Ret_Handled;

        case Event_Time_500ms:
            return EOS_TRAN(state_off);

        default:
            return EOS_SUPER(eos_state_top);
    }
}

static eos_ret_t state_off(eos_led_t * const me, eos_event_t const * const e)
{
    switch (e->topic) {
        case Event_Enter:
            printf("State Off!\n");
            me->status = 0;
            return EOS_Ret_Handled;

        case Event_Time_500ms:
            return EOS_TRAN(state_on);

        default:
            return EOS_SUPER(eos_state_top);
    }
}

