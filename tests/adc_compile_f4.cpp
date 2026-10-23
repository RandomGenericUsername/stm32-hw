// Compile check for the F4 ADC driver path (NUCLEO-F411RE reference).
// Compile with -mcpu=cortex-m4 -DSTM32F411xE and the F4 CMSIS includes.

#include <cstdint>

#include "stm32/drivers/f4.hpp"

struct AdcHandler {
    void on_complete(std::uint16_t value) {
        (void)value;
    }
};

using Adc1 = stm32::drivers::f4::Adc1<AdcHandler>;

extern "C" std::uint16_t adc_f4_check() {
    Adc1::init();
    Adc1::set_irq_priority(15);
    Adc1::start<0>(Adc1::SampleTime::Cycles56);
    return Adc1::ready() ? Adc1::take() : 0u;
}
