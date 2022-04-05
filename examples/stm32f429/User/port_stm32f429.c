#include "eventos.h"
#include "rtt/SEGGER_RTT.h"

void eos_task_start(    eos_task_t * const me,
                        eos_func_t func,
                        void *stack_addr,
                        uint32_t stack_size)
{
    *(uint32_t volatile *)0xE000ED20 |= (0xFFU << 16U);
    
    /* round down the stack top to the 8-byte boundary
     * NOTE: ARM Cortex-M stack grows down from hi -> low memory
     */
    uint32_t *sp = (uint32_t *)((((uint32_t)stack_addr + stack_size) >> 3U) << 3U);
    uint32_t *stk_limit;

    *(-- sp) = (uint32_t)(1 << 24);            /* xPSR, Set Bit24(Thumb Mode) to 1. */
    *(-- sp) = (uint32_t)func;                 /* the entry function (PC) */
    *(-- sp) = (uint32_t)func;                 /* R14(LR) */
    *(-- sp) = (uint32_t)0x12121212u;          /* R12 */
    *(-- sp) = (uint32_t)0x03030303u;          /* R3 */
    *(-- sp) = (uint32_t)0x02020202u;          /* R2 */
    *(-- sp) = (uint32_t)0x01010101u;          /* R1 */
    *(-- sp) = (uint32_t)0x00000000u;          /* R0 */
    /* additionally, fake registers R4-R11 */
    *(-- sp) = (uint32_t)0x11111111u;          /* R11 */
    *(-- sp) = (uint32_t)0x10101010u;          /* R10 */
    *(-- sp) = (uint32_t)0x09090909u;          /* R9 */
    *(-- sp) = (uint32_t)0x08080808u;          /* R8 */
    *(-- sp) = (uint32_t)0x07070707u;          /* R7 */
    *(-- sp) = (uint32_t)0x06060606u;          /* R6 */
    *(-- sp) = (uint32_t)0x05050505u;          /* R5 */
    *(-- sp) = (uint32_t)0x04040404u;          /* R4 */

    /* save the top of the stack in the task's attibute */
    me->sp = sp;

    /* round up the bottom of the stack to the 8-byte boundary */
    stk_limit = (uint32_t *)(((((uint32_t)stack_addr - 1U) >> 3U) + 1U) << 3U);

    /* pre-fill the unused part of the stack with 0xDEADBEEF */
    for (sp = sp - 1U; sp >= stk_limit; --sp) {
        *sp = 0xDEADBEEFU;
    }
}

__asm void PendSV_Handler(void)
{
    IMPORT        eos_current           /* extern variable */
    IMPORT        eos_next              /* extern variable */

#if (__TARGET_ARCH_THUMB == 3)          /* Cortex-M0/M0+/M1 (v6-M, v6S-M)? */
    CPSID   i                           /* disable interrupts (set PRIMASK) */
#else
    MOVS    r0,#0x3F
    CPSID   i                           /* selectively disable interrutps with BASEPRI */
    MSR     BASEPRI,r0                  /* apply the workaround the Cortex-M7 erraturm */
    CPSIE   i                           /* 837070, see SDEN-1068427. */
#endif                                  /* M3/M4/M7 */

    LDR           r1,=eos_current       /* if (eos_current != 0) { */
    LDR           r1,[r1,#0x00]
    CBZ           r1,PendSV_restore

    PUSH          {r4-r11}              /*     push r4-r11 into stack */
    LDR           r1,=eos_current       /*     eos_current->sp = sp; */
    LDR           r1,[r1,#0x00]
    STR           sp,[r1,#0x00]         /* } */
    
PendSV_restore
    LDR           r1,=eos_next          /* sp = eos_next->sp; */
    LDR           r1,[r1,#0x00]
    LDR           sp,[r1,#0x00]

    LDR           r1,=eos_next          /* eos_current = eos_next; */
    LDR           r1,[r1,#0x00]
    LDR           r2,=eos_current
    STR           r1,[r2,#0x00]
    POP           {r4-r11}              /* pop registers r4-r11 */
#if (__TARGET_ARCH_THUMB == 3)          /* Cortex-M0/M0+/M1 (v6-M, v6S-M)? */
    CPSIE   i                           /* enable interrupts (clear PRIMASK) */
#else                                   /* M3/M4/M7 */
    MOVS    r0,#0
    MSR     BASEPRI,r0                  /* enable interrupts (clear BASEPRI) */
    DSB                                 /* ARM Erratum 838869 */
#endif                                  /* M3/M4/M7 */
    BX            lr                    /* return to the next task */
}

int32_t critical_count = 0;
void eos_port_critical_enter(void)
{
    __disable_irq();
    critical_count ++;
}

void eos_port_task_switch(void)
{
    *(uint32_t volatile *)0xE000ED04 = (1U << 28);
}

void eos_port_critical_exit(void)
{
    critical_count --;
    if (critical_count <= 0) {
        critical_count = 0;
        __enable_irq();
    }
}

uint32_t eos_error_id = 0;
void eos_port_assert(uint32_t error_id)
{
    eos_error_id = error_id;

    while (1) {
    }
}

uint8_t flag_pub = 0;
void eos_hook_idle(void)
{
    if (flag_pub != 0) {
        flag_pub = 0;
        eos_event_pub_topic("Event_Time_1000ms");
    }
}

void eos_hook_start(void)
{

}

void eos_hook_stop(void)
{

}
