// Compile check for the WL55 ADC driver path (NUCLEO-WL55JC reference).
// Compile with -mcpu=cortex-m4 -DSTM32WL55xx and the WL55 CMSIS includes.

#include <cstdint>

#include "stm32/drivers/wl55.hpp"

struct AdcHandler {
    void on_complete(std::uint16_t value) {
        (void)value;
    }
};

using Adc1 = stm32::drivers::wl55::Adc1<AdcHandler>;

static_assert(stm32::drivers::AdcHandler<AdcHandler>);
static_assert(stm32::drivers::AdcHandler<stm32::drivers::AdcNullHandler>);

struct NotAnAdcHandler {};
static_assert(!stm32::drivers::AdcHandler<NotAnAdcHandler>);

// Pure conversion: VREF+ = VREF+_Charac * VREFINT_CAL / VREFINT_DATA.
static_assert(Adc1::vrefplus_millivolts(1515u, 1515u) == 3300u,
              "calibration sample must map back to 3.3 V");
static_assert(Adc1::vrefplus_millivolts(1515u, 1650u) == 3030u,
              "rounding must not overflow 32-bit arithmetic");
static_assert(Adc1::vrefplus_millivolts(1515u, 0u) == 0u,
              "zero raw sample must not divide by zero");

extern "C" std::uint16_t adc_wl55_check() {
    Adc1::init();
    Adc1::enable_vrefint();
    Adc1::set_irq_priority(15);
    Adc1::start<0>(Adc1::SampleTime::Cycles12_5);

    const std::uint16_t value = Adc1::ready() ? Adc1::take() : 0u;
    (void)Adc1::vrefint_cal();
    (void)Adc1::vrefplus_millivolts(value);
    return value;
}
