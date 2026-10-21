// Compile check for the WL55 ADC family path (NUCLEO-WL55JC reference), EOC IRQ
// + stateful functor callback.
// Compile with -mcpu=cortex-m4 -DSTM32WL55xx and the WL55 CMSIS includes.

#include <cstdint>

#include "stm32/device/wl55/adc.hpp"
#include "stm32/drivers/adc_sampler.hpp"
#include "stm32/hw/adc/wl55.hpp"

using Adc1 = stm32::hw::WlAdc<stm32::device::wl55::Adc1>;

struct WlState {
    std::uint16_t value = 0;
    std::uint32_t context = 42;
};

template <class State>
struct WlCallback {
    State& state;

    void operator()(std::uint16_t value) const {
        state.value = value;
    }
};

using Sampler =
    stm32::drivers::AdcSampler<Adc1, WlCallback<WlState>>;

static WlState g_state;
static Sampler g_sampler(WlCallback<WlState>{g_state});

extern "C" std::uint16_t adc_wl55_check() {
    g_sampler.init();
    g_sampler.set_irq_priority(15);
    g_sampler.start<0>(Adc1::SampleTime::Cycles12_5);
    (void)g_sampler.on_irq();
    return g_state.value;
}
