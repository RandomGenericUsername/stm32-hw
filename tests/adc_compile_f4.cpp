// Compile check for the F4 ADC family path (NUCLEO-F411RE reference).
// Compile with -mcpu=cortex-m4 -DSTM32F411xE and the F4 CMSIS includes.

#include <cstdint>

#include "stm32/device/f4/adc.hpp"
#include "stm32/hw/adc/f4.hpp"

using Adc1 = stm32::hw::F4Adc<stm32::device::f4::Adc1>;

extern "C" std::uint16_t adc_f4_check() {
    Adc1::init();
    Adc1::configure<0>(Adc1::SampleTime::Cycles56);
    const std::uint16_t value = Adc1::read<0>();
    Adc1::disable();
    return value;
}
