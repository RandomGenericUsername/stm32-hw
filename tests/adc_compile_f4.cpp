// Compile check for the F4 ADC family path (NUCLEO-F411RE reference), EOC IRQ
// + stateful functor callback.
// Compile with -mcpu=cortex-m4 -DSTM32F411xE and the F4 CMSIS includes.

#include <cstdint>

#include "stm32/device/f4/adc.hpp"
#include "stm32/drivers/adc_sampler.hpp"
#include "stm32/hw/adc/f4.hpp"

using Adc1 = stm32::hw::F4Adc<stm32::device::f4::Adc1>;

struct F4State {
    std::uint16_t value = 0;
};

template <class State>
struct F4Callback {
    State& state;

    void operator()(std::uint16_t value) const {
        state.value = value;
    }
};

using Callback = F4Callback<F4State>;
using Sampler = stm32::drivers::AdcSampler<Adc1, Callback>;

static F4State g_state;
static Sampler g_sampler(Callback{g_state});

extern "C" std::uint16_t adc_f4_check() {
    g_sampler.init();
    g_sampler.set_irq_priority(15);
    g_sampler.start<0>(Adc1::SampleTime::Cycles56);
    (void)g_sampler.on_irq();
    return g_state.value;
}
