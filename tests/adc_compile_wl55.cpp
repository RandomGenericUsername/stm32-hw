// Compile check for the WL55 ADC family path (NUCLEO-WL55JC reference), EOC IRQ
// + constructor-injected completion callback.
// Compile with -mcpu=cortex-m4 -DSTM32WL55xx and the WL55 CMSIS includes.

#include <cstdint>

#include "stm32/device/wl55/adc.hpp"
#include "stm32/drivers/adc_sampler.hpp"
#include "stm32/hw/adc/wl55.hpp"

using Adc1 = stm32::hw::WlAdc<stm32::device::wl55::Adc1>;

void on_adc_wl55(std::uint16_t value, std::uint32_t context) {
    (void)value;
    (void)context;
}

using Sampler =
    stm32::drivers::AdcSampler<Adc1, decltype(&on_adc_wl55), std::uint32_t>;

extern "C" std::uint16_t adc_wl55_check() {
    static Sampler sampler(&on_adc_wl55, 42u);
    sampler.init();
    sampler.set_irq_priority(15);
    sampler.start<0>(Adc1::SampleTime::Cycles12_5);
    (void)sampler.on_irq();
    return sampler.ready() ? sampler.take() : 0u;
}
