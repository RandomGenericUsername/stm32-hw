// Compile check for the F4 ADC family path (NUCLEO-F411RE reference), EOC IRQ
// + variadic callback registration.
// Compile with -mcpu=cortex-m4 -DSTM32F411xE and the F4 CMSIS includes.

#include <cstdint>

#include "stm32/device/f4/adc.hpp"
#include "stm32/drivers/adc_sampler.hpp"
#include "stm32/hw/adc/f4.hpp"

using Adc1 = stm32::hw::F4Adc<stm32::device::f4::Adc1>;
using Sampler = stm32::drivers::AdcSampler<Adc1>;

void on_adc_f4(Sampler* self) {
    (void)self->take();
}

extern "C" std::uint16_t adc_f4_check() {
    static Sampler sampler;
    sampler.init();
    sampler.set_irq_priority(15);
    sampler.on_complete<&on_adc_f4>(&sampler);
    sampler.start<0>(Adc1::SampleTime::Cycles56);
    (void)sampler.on_irq();
    return sampler.ready() ? sampler.take() : 0u;
}
