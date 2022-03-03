// state -----------------------------------------------------------------------
#include "eventos.h"
#include "eos_test.h"
#include "event_def.h"

// state -----------------------------------------------------------------------
static eos_ret_t state_init(fsm_t * const me, eos_event_t const * const e);
static eos_ret_t state_on(fsm_t * const me, eos_event_t const * const e);
static eos_ret_t state_off(fsm_t * const me, eos_event_t const * const e);

// api -------------------------------------------------------------------------
void fsm_init(fsm_t * const me, eos_u8_t priority, void const * const parameter)
{
    me->state = 0;
    me->count = 0;

    eos_sm_init(&me->super, priority, parameter);
    eos_sm_start(&me->super, EOS_STATE_CAST(state_init));
}

eos_u32_t fsm_state(fsm_t * const me)
{
    return me->state;
}

eos_u32_t fsm_event_count(fsm_t * const me)
{
    return me->count;
}

void fsm_reset_event_count(fsm_t * const me)
{
    me->count = 0;
}

// state function --------------------------------------------------------------
static eos_ret_t state_init(fsm_t * const me, eos_event_t const * const e)
{
    (void)e;

#if (EOS_USE_PUB_SUB != 0)
    EOS_EVENT_SUB(Event_Time_500ms);
    EOS_EVENT_SUB(Event_TestFsm);
#endif

    me->state = 0;
    me->count = 0;

    return EOS_TRAN(state_off);
}

static eos_ret_t state_off(fsm_t * const me, eos_event_t const * const e)
{
    switch (e->topic) {
        case Event_Enter:
            me->state = 0;
            return EOS_Ret_Handled;

        case Event_TestFsm:
            me->count ++;
            return EOS_TRAN(state_on);

        case Event_Time_500ms:
            me->count ++;
            return EOS_TRAN(state_on);

        default:
            return EOS_SUPER(eos_state_top);
    }
}

static eos_ret_t state_on(fsm_t * const me, eos_event_t const * const e)
{
    switch (e->topic) {
        case Event_Enter: {
            me->state = 1;
            return EOS_Ret_Handled;
        }

        case Event_Exit:
            return EOS_Ret_Handled;

        case Event_TestFsm:
            me->count ++;
            return EOS_TRAN(state_off);

        case Event_Time_500ms:
            me->count ++;
            return EOS_TRAN(state_off);

        default:
            return EOS_SUPER(eos_state_top);
    }
}
