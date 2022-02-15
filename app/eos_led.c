#include "eos_led.h"
#include "meow.h"

typedef struct eos_led_tag {
    m_obj_t super;

    uint8_t status;
} eos_led_t;

static eos_led_t led;

// state -----------------------------------------------------------------------
static m_ret_t state_init(eos_led_t * const me, m_evt_t const * const e);
static m_ret_t state_on(eos_led_t * const me, m_evt_t const * const e);
static m_ret_t state_off(eos_led_t * const me, m_evt_t const * const e);

void eos_led_init(void)
{
    obj_init(&led.super, "SmLed", ObjType_Sm, 1, 32);
    sm_start(&led.super, STATE_CAST(state_init));

    led.status = 0;
}

// state -----------------------------------------------------------------------
static m_ret_t state_init(eos_led_t * const me, m_evt_t const * const e)
{
    EVT_SUB(Evt_Time_500ms);

    return M_TRAN(state_off);
}

static m_ret_t state_on(eos_led_t * const me, m_evt_t const * const e)
{
    switch (e->sig) {
        case Evt_Enter:
            me->status = 1;
            return M_Ret_Handled;

        case Evt_Time_500ms:
            return M_TRAN(state_off);

        default:
            return M_SUPER(m_state_top);
    }
}

static m_ret_t state_off(eos_led_t * const me, m_evt_t const * const e)
{
    switch (e->sig) {
        case Evt_Enter:
            me->status = 0;
            return M_Ret_Handled;

        case Evt_Time_500ms:
            return M_TRAN(state_on);

        default:
            return M_SUPER(m_state_top);
    }
}

