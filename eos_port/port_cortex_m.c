#include "eventos.h"

void eos_task_start_private(eos_task_t * const me,
                            eos_func_t func,
                            uint8_t priority,
                            void *stack_addr,
                            uint32_t stack_size)
{
    // Set PendSV to be the lowest priority.
    *(uint32_t volatile *)0xE000ED20 |= (0xFFU << 16U);
    
    uint32_t mod = (uint32_t)me->stack % 4;
    if (mod == 0) {
        me->stack = stack_addr;
        me->size = stack_size;
    }
    else {
        me->stack = (void *)((uint32_t)stack_addr - mod);
        me->size = stack_size - 4;
    }
    /* pre-fill the unused part of the stack with 0xDEADBEEF */
    for (uint32_t i = 0; i < (me->size / 4); i ++) {
        ((uint32_t *)me->stack)[i] = 0xDEADBEEFU;
    }
    
    /* round down the stack top to the 8-byte boundary
     * NOTE: ARM Cortex-M stack grows down from hi -> low memory
     */
    uint32_t *sp = (uint32_t *)((((uint32_t)stack_addr + stack_size) >> 3U) << 3U);

    *(-- sp) = (uint32_t)(1 << 24);            /* xPSR, Set Bit24(Thumb Mode) to 1. */
    *(-- sp) = (uint32_t)func;                 /* the entry function (PC) */
    *(-- sp) = (uint32_t)func;                 /* R14(LR) */
    *(-- sp) = (uint32_t)0x12121212u;          /* R12 */
    *(-- sp) = (uint32_t)0x03030303u;          /* R3 */
    *(-- sp) = (uint32_t)0x02020202u;          /* r2 */
    *(-- sp) = (uint32_t)0x01010101u;          /* R1 */
    *(-- sp) = (uint32_t)0x00000000u;          /* r0 */
    /* additionally, fake registers r4-r11 */
    *(-- sp) = (uint32_t)0x11111111u;          /* r11 */
    *(-- sp) = (uint32_t)0x10101010u;          /* r10 */
    *(-- sp) = (uint32_t)0x09090909u;          /* r9 */
    *(-- sp) = (uint32_t)0x08080808u;          /* r8 */
    *(-- sp) = (uint32_t)0x07070707u;          /* r7 */
    *(-- sp) = (uint32_t)0x06060606u;          /* r6 */
    *(-- sp) = (uint32_t)0x05050505u;          /* r5 */
    *(-- sp) = (uint32_t)0x04040404u;          /* r4 */

    /* save the top of the stack in the task's attibute */
    me->sp = sp;
    me->priority = priority;
}

/* Interrupt service function ----------------------------------------------- */
#if (defined __CC_ARM)
__asm void PendSV_Handler(void)
{
    IMPORT        eos_current           /* extern variable */
    IMPORT        eos_next              /* extern variable */

    CPSID         i                     /* disable interrupts (set PRIMASK) */

    LDR           r1,=eos_current       /* if (eos_current != 0) { */
    LDR           r1,[r1,#0x00]
#if (__TARGET_ARCH_THUMB == 3)          /* Cortex-M0/M0+/M1 (v6-M, v6S-M)? */
    CMP           r1, #0
    BEQ           PendSV_restore
    NOP
    PUSH          {r4-r7}              /*     push r4-r11 into stack */
    MOV           r4, r8
    MOV           r5, r9
    MOV           r6, r10
    MOV           r7, r11
    PUSH          {r4-r7}
#else
    CBZ           r1,PendSV_restore
    PUSH          {r4-r11}              /*     push r4-r11 into stack */
#endif
  
    LDR           r1,=eos_current       /*     eos_current->sp = sp; */
    LDR           r1,[r1,#0x00]
#if (__TARGET_ARCH_THUMB == 3)          /* Cortex-M0/M0+/M1 (v6-M, v6S-M)? */
    MOV           r2, SP
    STR           r2,[r1,#0x00]         /* } */
#else
    STR           SP,[r1,#0x00]
#endif
    
PendSV_restore
    LDR           r1,=eos_next          /* sp = eos_next->sp; */
    LDR           r1,[r1,#0x00]
#if (__TARGET_ARCH_THUMB == 3)          /* Cortex-M0/M0+/M1 (v6-M, v6S-M)? */
    LDR           r0,[r1,#0x00]
    MOV           SP, r0
#else
    LDR           sp,[r1,#0x00]
#endif
    LDR           r1,=eos_next          /* eos_current = eos_next; */
    LDR           r1,[r1,#0x00]
    LDR           r2,=eos_current
    STR           r1,[r2,#0x00]
#if (__TARGET_ARCH_THUMB == 3)          /* Cortex-M0/M0+/M1 (v6-M, v6S-M)? */
    POP           {r4-r7}
    MOV           r8, r4
    MOV           r9, r5
    MOV           r10,r6
    MOV           r11,r7
    POP           {r4-r7}
#else
    POP           {r4-r11}              /* pop registers r4-r11 */
#endif
    CPSIE         i                     /* enable interrupts (clear PRIMASK) */
    BX            lr                    /* return to the next task */
}
#endif

/*******************************************************************************
* NOTE:
* The inline GNU assembler does not accept mnemonics MOVS, LSRS and ADDS,
* but for Cortex-M0/M0+/M1 the mnemonics MOV, LSR and ADD always set the
* condition flags in the PSR.
*******************************************************************************/
#if ((defined __GNUC__) || (defined __ICCARM__))
#if (defined __GNUC__)
__attribute__ ((naked))
#endif
#if ((defined __ICCARM__))
__stackless
#endif
void PendSV_Handler(void)
{
    __asm volatile
    (
    "CPSID         i                \n" /* disable interrupts (set PRIMASK) */
    "LDR           r1,=eos_current  \n"  /* if (eos_current != 0) { */
    "LDR           r1,[r1,#0x00]    \n"

#if (__TARGET_ARCH_THUMB == 3)          /* Cortex-M0/M0+/M1 (v6-M, v6S-M)? */
    "CMP           r1, #0           \n"
    "BEQ           restore          \n"
    "NOP                            \n"
    "PUSH          {r4-r7}          \n" /*      push r4-r11 into stack */
    "MOV           r4, r8           \n"
    "MOV           r5, r9           \n"
    "MOV           r6, r10          \n"
    "MOV           r7, r11          \n"
    "PUSH          {r4-r7}          \n"
#else
    "CBZ           r1,restore       \n"
    "PUSH          {r4-r11}         \n"
#endif

    "LDR           r1,=eos_current  \n"  /*     eos_current->sp = sp; */
    "LDR           r1,[r1,#0x00]    \n"

#if (__TARGET_ARCH_THUMB == 3)          /* Cortex-M0/M0+/M1 (v6-M, v6S-M)? */
    "MOV           r2, SP           \n"
    "STR           r2,[r1,#0x00]    \n"  /* } */
#else
    "STR           sp,[r1,#0x00]    \n"  /* } */
#endif

    "restore: LDR r1,=eos_next      \n"  /* sp = eos_next->sp; */
    "LDR           r1,[r1,#0x00]    \n"
#if (__TARGET_ARCH_THUMB == 3)          /* Cortex-M0/M0+/M1 (v6-M, v6S-M)? */
    "LDR           r0,[r1,#0x00]    \n"
    "MOV           SP, r0           \n"
#else
    "LDR           sp,[r1,#0x00]    \n"
#endif

    "LDR           r1,=eos_next     \n"  /* eos_current = eos_next; */
    "LDR           r1,[r1,#0x00]    \n"
    "LDR           r2,=eos_current  \n"
    "STR           r1,[r2,#0x00]    \n"
#if (__TARGET_ARCH_THUMB == 3)          /* Cortex-M0/M0+/M1 (v6-M, v6S-M)? */
    "POP           {r4-r7}          \n"
    "MOV           r8, r4           \n"
    "MOV           r9, r5           \n"
    "MOV           r10,r6           \n"
    "MOV           r11,r7           \n"
    "POP           {r4-r7}          \n"
#else
    "POP           {r4-r11}         \n"  /* pop registers r4-r11 */
#endif
    "CPSIE         i                \n"  /* enable interrupts (clear PRIMASK) */
    "BX            lr               \n"   /* return to the next task */
    );
}
#endif

static int32_t critical_count = 0;
#if (defined __CC_ARM)
inline void eos_critical_enter(void)
#elif ((defined __GNUC__) || (defined __ICCARM__))
__attribute__((always_inline)) inline void eos_critical_enter(void)
#endif
{
#if (defined __CC_ARM)
    __disable_irq();
#elif ((defined __GNUC__) || (defined __ICCARM__))
    __asm volatile ("cpsid i" : : : "memory");
#endif
    critical_count ++;
}

#if (defined __CC_ARM)
inline void eos_critical_exit(void)
#elif ((defined __GNUC__) || (defined __ICCARM__))
__attribute__((always_inline)) inline void eos_critical_exit(void)
#endif
{
    critical_count --;
    if (critical_count <= 0) {
        critical_count = 0;
#if (defined __CC_ARM)
        __enable_irq();
#elif ((defined __GNUC__) || (defined __ICCARM__))
        __asm volatile ("cpsie i" : : : "memory");
#endif
    }
}

void eos_port_task_switch(void)
{
    *(uint32_t volatile *)0xE000ED04 = (1U << 28);
}

uint32_t eos_error_id = 0;
void eos_port_assert(uint32_t error_id)
{
    eos_error_id = error_id;

    while (1) {
    }
}
