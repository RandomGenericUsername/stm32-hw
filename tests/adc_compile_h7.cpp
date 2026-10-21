// Compile check for the H7 ADC family path (NUCLEO-H755ZI-Q reference), EOC IRQ
// + stateful functor callbacks for ADC1 and ADC3.
// Compile with -mcpu=cortex-m7 -DSTM32H755xx -DCORE_CM7 and H7 CMSIS includes.

#include <cstdint>

#include "stm32/device/h7/adc.hpp"
#include "stm32/drivers/adc_sampler.hpp"
#include "stm32/hw/adc/h7.hpp"

using Adc1 = stm32::hw::H7Adc<stm32::device::h7::Adc1>;
using Adc3 = stm32::hw::H7Adc<stm32::device::h7::Adc3>;

struct H7State1 {
    std::uint16_t value = 0;
    std::uint32_t context = 7;
};

struct H7State3 {
    std::uint16_t value = 0;
};

template <class State>
struct H7Callback {
    State& state;

    void operator()(std::uint16_t value) const {
        state.value = value;
    }
};

using Sampler1 =
    stm32::drivers::AdcSampler<Adc1, H7Callback<H7State1>>;
using Sampler3 =
    stm32::drivers::AdcSampler<Adc3, H7Callback<H7State3>>;

static H7State1 g_state1;
static H7State3 g_state3;
static Sampler1 g_sampler1(H7Callback<H7State1>{g_state1});
static Sampler3 g_sampler3(H7Callback<H7State3>{g_state3});

extern "C" std::uint16_t adc_h7_check() {
    g_sampler1.init();
    g_sampler1.set_irq_priority(15);
    g_sampler1.start<0>(Adc1::SampleTime::Cycles12_5);

    g_sampler3.init();
    g_sampler3.set_irq_priority(15);
    g_sampler3.start<0>(Adc3::SampleTime::Cycles12_5);

    (void)g_sampler1.on_irq();
    (void)g_sampler3.on_irq();

    return static_cast<std::uint16_t>(g_state1.value ^ g_state3.value);
}
