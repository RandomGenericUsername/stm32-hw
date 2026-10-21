// Compile check for the WL55 ADC family path (NUCLEO-WL55JC reference).
// Compile with -mcpu=cortex-m4 -DSTM32WL55xx and the WL55 CMSIS includes.

#include <cstdint>

#include "stm32/device/wl55/adc.hpp"
#include "stm32/hw/adc/wl55.hpp"

using Adc1 = stm32::hw::WlAdc<stm32::device::wl55::Adc1>;

extern "C" std::uint16_t adc_wl55_check() {
    Adc1::init();
    Adc1::configure<0>(Adc1::SampleTime::Cycles12_5);
    const std::uint16_t value = Adc1::read<0>();
    Adc1::disable();
    return value;
}
