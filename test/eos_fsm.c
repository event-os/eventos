// state -----------------------------------------------------------------------
#include "eventos.h"
#include "eos_test.h"

#if (EOS_USE_SM_MODE != 0)
// state -----------------------------------------------------------------------
static eos_ret_t state_init(fsm_t * const me, eos_event_t const * const e);
static eos_ret_t state_on(fsm_t * const me, eos_event_t const * const e);
static eos_ret_t state_off(fsm_t * const me, eos_event_t const * const e);

// api -------------------------------------------------------------------------
void fsm_init(  fsm_t * const me,
                const char *name,
                uint8_t priority,
                void const *stack, uint32_t size)
{
    me->state = 0;
    me->count = 0;

    eos_sm_init(&me->super, name, priority, stack, size);
    eos_sm_start(&me->super, EOS_STATE_CAST(state_init));
}

uint32_t fsm_state(fsm_t * const me)
{
    return me->state;
}

uint32_t fsm_event_count(fsm_t * const me)
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
    EOS_EVENT_SUB("Event_Time_500ms");
    EOS_EVENT_SUB("Event_TestFsm");
#endif

    me->state = 0;
    me->count = 0;

    return EOS_TRAN(state_off);
}

static eos_ret_t state_off(fsm_t * const me, eos_event_t const * const e)
{
    if (eos_event_topic(e, "Event_Enter")) {
        me->state = 0;
        return EOS_Ret_Handled;
    }

    if (eos_event_topic(e, "Event_Exit")) {
        return EOS_Ret_Handled;
    }

    if (eos_event_topic(e, "Event_TestFsm")) {
        me->count ++;
        return EOS_TRAN(state_on);
    }

    if (eos_event_topic(e, "Event_Time_500ms")) {
        me->count ++;
        return EOS_TRAN(state_on);
    }

    return EOS_SUPER(eos_state_top);
}

static eos_ret_t state_on(fsm_t * const me, eos_event_t const * const e)
{
    if (eos_event_topic(e, "Event_Enter")) {
        me->state = 1;
        return EOS_Ret_Handled;
    }

    if (eos_event_topic(e, "Event_Exit")) {
        return EOS_Ret_Handled;
    }

    if (eos_event_topic(e, "Event_TestFsm")) {
        me->count ++;
        return EOS_TRAN(state_on);
    }

    if (eos_event_topic(e, "Event_Time_500ms")) {
        me->count ++;
        return EOS_TRAN(state_on);
    }

    return EOS_SUPER(eos_state_top);
}

#endif
