// Compile check for the H7 ADC family path (NUCLEO-H755ZI-Q reference).
// Compile with -mcpu=cortex-m7 -DSTM32H755xx -DCORE_CM7 and H7 CMSIS includes.

#include <cstdint>

#include "stm32/device/h7/adc.hpp"
#include "stm32/hw/adc/h7.hpp"

using Adc1 = stm32::hw::H7Adc<stm32::device::h7::Adc1>;
using Adc3 = stm32::hw::H7Adc<stm32::device::h7::Adc3>;

extern "C" std::uint16_t adc_h7_check() {
    Adc1::init();
    Adc1::configure<0>(Adc1::SampleTime::Cycles12_5);
    std::uint16_t value = Adc1::read<0>();
    Adc1::disable();

    Adc3::init();
    Adc3::configure<0>(Adc3::SampleTime::Cycles12_5);
    value ^= Adc3::read<0>();
    Adc3::disable();
    return value;
}
