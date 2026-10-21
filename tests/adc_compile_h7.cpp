// Compile check for the H7 ADC family path (NUCLEO-H755ZI-Q reference), EOC IRQ
// + constructor-injected completion callback for ADC1 and ADC3.
// Compile with -mcpu=cortex-m7 -DSTM32H755xx -DCORE_CM7 and H7 CMSIS includes.

#include <cstdint>

#include "stm32/device/h7/adc.hpp"
#include "stm32/drivers/adc_sampler.hpp"
#include "stm32/hw/adc/h7.hpp"

using Adc1 = stm32::hw::H7Adc<stm32::device::h7::Adc1>;
using Adc3 = stm32::hw::H7Adc<stm32::device::h7::Adc3>;

void on_adc_h7_1(std::uint16_t value, std::uint32_t context) {
    (void)value;
    (void)context;
}

void on_adc_h7_3(std::uint16_t value) {
    (void)value;
}

using Sampler1 =
    stm32::drivers::AdcSampler<Adc1, decltype(&on_adc_h7_1), std::uint32_t>;
using Sampler3 =
    stm32::drivers::AdcSampler<Adc3, decltype(&on_adc_h7_3)>;

extern "C" std::uint16_t adc_h7_check() {
    static Sampler1 sampler1(&on_adc_h7_1, 7u);
    static Sampler3 sampler3(&on_adc_h7_3);

    sampler1.init();
    sampler1.set_irq_priority(15);
    sampler1.start<0>(Adc1::SampleTime::Cycles12_5);

    sampler3.init();
    sampler3.set_irq_priority(15);
    sampler3.start<0>(Adc3::SampleTime::Cycles12_5);

    (void)sampler1.on_irq();
    (void)sampler3.on_irq();

    std::uint16_t value = sampler1.ready() ? sampler1.take() : 0u;
    value ^= sampler3.ready() ? sampler3.take() : 0u;
    return value;
}
