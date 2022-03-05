/* include ------------------------------------------------------------------ */
#include "eos_test.h"
#include "eventos.h"
#include "event_def.h"

/* event handler ------------------------------------------------------------ */
static void reactor_func(reactor_t * const me, eos_event_t const * const e);

/* api ---------------------------------------------------------------------- */
void reactor_init(reactor_t * const me, eos_u8_t priority, void const * const parameter)
{
    eos_reactor_init(&me->super, priority, parameter);
    eos_reactor_start(&me->super, EOS_HANDLER_CAST(reactor_func));

#if (EOS_USE_PUB_SUB != 0)
    EOS_EVENT_SUB(Event_TestReactor);
    EOS_EVENT_SUB(Event_Test);
#endif
}

int reactor_e_test_count(reactor_t * const me)
{
    return me->count_test;
}

int reactor_e_tr_count(reactor_t * const me)
{
    return me->count_tr;
}

/* event handler ------------------------------------------------------------ */
static void reactor_func(reactor_t * const me, eos_event_t const * const e)
{
    if (e->topic == Event_TestReactor) {
        me->count_tr ++;
    }

    if (e->topic == Event_Test) {
        me->count_test ++;
    }
}
