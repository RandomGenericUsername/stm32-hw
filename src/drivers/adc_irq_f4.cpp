// F4 ADC interrupt ownership. ADC1/2/3 share ADC_IRQn.
#include <stm32f4xx.h>

#include "stm32/drivers/adc_irq.hpp"

extern "C" void ADC_IRQHandler(void) {
    stm32::drivers::AdcIrqRouter<ADC_IRQn>::dispatch();
}
