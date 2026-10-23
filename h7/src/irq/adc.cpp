// H7 ADC interrupt ownership. ADC1/2 share ADC_IRQn; ADC3 has its own vector.
#include <stm32h7xx.h>

#include "stm32/core/drivers/adc_irq.hpp"

extern "C" void ADC_IRQHandler(void) {
    stm32::drivers::AdcIrqRouter<ADC_IRQn>::dispatch();
}

extern "C" void ADC3_IRQHandler(void) {
    stm32::drivers::AdcIrqRouter<ADC3_IRQn>::dispatch();
}
