
#include "meow.h"
#include "stdio.h"
#include "led.h"
#include "bsp.h"

led_t led;
int main(void) 
{
    m_irq_disable();
    
    bsp_init();
    systick_enable();
    
    m_irq_enable();
    
    meow_init();
    
    led_init(&led, "led", 0);

    meow_run();

    return 0;
}
