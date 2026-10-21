// Compile check for the WL55 ADC family path (NUCLEO-WL55JC reference), EOC IRQ
// + variadic callback registration.
// Compile with -mcpu=cortex-m4 -DSTM32WL55xx and the WL55 CMSIS includes.

#include <cstdint>

#include "stm32/device/wl55/adc.hpp"
#include "stm32/drivers/adc_sampler.hpp"
#include "stm32/hw/adc/wl55.hpp"

using Adc1 = stm32::hw::WlAdc<stm32::device::wl55::Adc1>;
using Sampler = stm32::drivers::AdcSampler<Adc1>;

void on_adc_wl55(Sampler* self, std::uint32_t context) {
    (void)context;
    (void)self->take();
}

extern "C" std::uint16_t adc_wl55_check() {
    static Sampler sampler;
    sampler.init();
    sampler.set_irq_priority(15);
    sampler.on_complete<&on_adc_wl55>(&sampler, 42u);
    sampler.start<0>(Adc1::SampleTime::Cycles12_5);
    (void)sampler.on_irq();
    return sampler.ready() ? sampler.take() : 0u;
}
