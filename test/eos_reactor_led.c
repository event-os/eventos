/* include ------------------------------------------------------------------ */
#include "eos_test.h"
#include "eventos.h"
#include <stdio.h>

/* event handler ------------------------------------------------------------ */
static void reactor_func(reactor_t * const me, eos_event_t const * const e);

/* api ---------------------------------------------------------------------- */
void reactor_init(reactor_t * const me, const char *name, uint8_t priority, void const *stack, uint32_t size)
{
    me->count_test = 0;
    me->count_tr = 0;
    me->status = false;
    me->value = 0;
    
    eos_reactor_init(&me->super, name, priority, stack, size);
    eos_reactor_start(&me->super, EOS_HANDLER_CAST(reactor_func));

    me->data_size = 0;
    me->count_test = 0;
    me->count_tr = 0;

#if (EOS_USE_PUB_SUB != 0)
    EOS_EVENT_SUB("Event_TestReactor");
    EOS_EVENT_SUB("Event_Test");
    EOS_EVENT_SUB("Event_Data");
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

uint32_t reactor_get_value(reactor_t * const me)
{
    return me->value;
}

/* event handler ------------------------------------------------------------ */
static void reactor_func(reactor_t * const me, eos_event_t const * const e)
{
    if (eos_event_topic(e, "Event_TestReactor")) {
        me->count_tr ++;
    }

    if (eos_event_topic(e, "Event_Test")) {
        me->count_test ++;
        me->data_size = e->size;

        printf("me->data_size : %d.\n", me->data_size);
    }

    if (eos_event_topic(e, "Event_Data")) {
        me->value = *((uint32_t *)e->data);
    }
}
