#include "eventos.h"
#include "rtt/SEGGER_RTT.h"

__asm void PendSV_Handler(void)
{
    IMPORT        eos_current           /* extern variable */
    IMPORT        eos_next              /* extern variable */

    CPSID         I                     /* disable irq */

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
    CPSIE         I                     /* enable irq */
    BX            lr                    /* return to the next task */
}

void eos_port_critical_enter(void)
{
    __disable_irq();
}


void eos_port_critical_exit(void)
{
    __enable_irq();
}

eos_u32_t eos_error_id = 0;
void eos_port_assert(eos_u32_t error_id)
{
    SEGGER_RTT_printf(0, "------------------------------------\n");
    SEGGER_RTT_printf(0, "ASSERT >>> Module: EventOS Nano, ErrorId: %d.\n", error_id);
    SEGGER_RTT_printf(0, "------------------------------------\n");
    
    eos_error_id = error_id;

    while (1) {
    }
}

void eos_hook_idle(void)
{
}

void eos_hook_start(void)
{

}

void eos_hook_stop(void)
{

}
