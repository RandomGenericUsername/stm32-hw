// Compile check for the WL55 ADC driver path (NUCLEO-WL55JC reference).
// Compile with -mcpu=cortex-m4 -DSTM32WL55xx and the WL55 CMSIS includes.

#include <cstdint>

#include "stm32/drivers/wl55.hpp"

using Adc1 = stm32::drivers::wl55::Adc1;

void on_adc_wl55(std::uint16_t value) {
    (void)value;
}

extern "C" std::uint16_t adc_wl55_check() {
    Adc1::init();
    Adc1::enable_vrefint();
    Adc1::on_complete(&on_adc_wl55);
    Adc1::set_irq_priority(15);
    Adc1::start<0>(Adc1::SampleTime::Cycles12_5);
    return Adc1::ready() ? Adc1::take() : 0u;
}
