
/* include ------------------------------------------------------------------ */
#include "eos_test.h"

/* **eos** ------------------------------------------------------------------ */
#if (EOS_USE_TIME_EVENT != 0)
typedef struct eos_event_timer {
    eos_topic_t topic;
    eos_bool_t is_one_shoot;
    eos_s32_t time_ms_delay;
    eos_u32_t timeout_ms;
} eos_event_timer_t;
#endif

typedef struct eos_block {
    struct eos_block *next;
    eos_u8_t free;
    eos_u16_t size;
} eos_block_t;

typedef struct eos_heap {
    eos_u8_t data[EOS_SIZE_HEAP];
    eos_block_t *list;
    eos_u16_t size;       /* total size */
    eos_u8_t error_id;
} eos_heap_t;

typedef struct eos_event_inner {
    eos_u32_t topic;
    void *data;
    eos_u32_t flag_sub;
} eos_event_inner_t;

typedef struct eos_tag {
    eos_mcu_t magic;
#if (EOS_USE_PUB_SUB != 0)
    eos_mcu_t *sub_table;                                     // event sub table
#endif

    eos_mcu_t actor_exist;
    eos_mcu_t actor_enabled;
    eos_actor_t * actor[EOS_MAX_ACTORS];

#if (EOS_USE_EVENT_DATA != 0)
    eos_heap_t heap;
#endif

#if (EOS_USE_TIME_EVENT != 0)
    eos_event_timer_t e_timer_pool[EOS_MAX_TIME_EVENT];
    eos_u32_t flag_etimerpool[EOS_MAX_TIME_EVENT / 32 + 1];    // timer pool flag
    eos_u32_t timeout_ms_min;
    eos_u32_t time_crt_ms;
    eos_bool_t etimerpool_empty;
#endif

    eos_bool_t enabled;
    eos_bool_t running;
    eos_bool_t idle;
} eos_t;
// **eos end** -----------------------------------------------------------------

void eos_test_event(void)
{
    
}
