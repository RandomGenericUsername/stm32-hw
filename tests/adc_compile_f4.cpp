// Compile check for the F4 ADC family path (NUCLEO-F411RE reference), EOC IRQ
// + constructor-injected completion callback.
// Compile with -mcpu=cortex-m4 -DSTM32F411xE and the F4 CMSIS includes.

#include <cstdint>

#include "stm32/device/f4/adc.hpp"
#include "stm32/drivers/adc_sampler.hpp"
#include "stm32/hw/adc/f4.hpp"

using Adc1 = stm32::hw::F4Adc<stm32::device::f4::Adc1>;

void on_adc_f4(std::uint16_t value) {
    (void)value;
}

using Sampler = stm32::drivers::AdcSampler<Adc1, decltype(&on_adc_f4)>;

extern "C" std::uint16_t adc_f4_check() {
    static Sampler sampler(&on_adc_f4);
    sampler.init();
    sampler.set_irq_priority(15);
    sampler.start<0>(Adc1::SampleTime::Cycles56);
    (void)sampler.on_irq();
    return sampler.ready() ? sampler.take() : 0u;
}
