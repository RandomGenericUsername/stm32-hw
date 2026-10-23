#pragma once

#include "stm32/f4/device/adc.hpp"
#include "stm32/core/drivers/adc.hpp"
#include "stm32/f4/adc/hardware.hpp"

namespace stm32::drivers::f4 {

template <AdcHandler Handler = AdcNullHandler>
using Adc1 = AdcDriver<hw::F4Adc<device::f4::Adc1>, Handler>;

} // namespace stm32::drivers::f4
