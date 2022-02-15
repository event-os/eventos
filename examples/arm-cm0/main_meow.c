
#include "meow.h"
#include "stdio.h"
#include "eos_led.h"
#include "bsp.h"

int main(void) 
{
    m_irq_disable();
    
    bsp_init();
    systick_enable();
    
    m_irq_enable();
    
    meow_init();
    
    eos_led_init();

    meow_run();

    return 0;
}
