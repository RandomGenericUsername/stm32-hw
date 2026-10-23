// WL55 ADC interrupt ownership. Compiled into the driver layer.
#include <stm32wl55xx.h>

#include "stm32/core/drivers/adc_irq.hpp"

extern "C" void ADC_IRQHandler(void) {
    stm32::drivers::AdcIrqRouter<ADC_IRQn>::dispatch();
}
