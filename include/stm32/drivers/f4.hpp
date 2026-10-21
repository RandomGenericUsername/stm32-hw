#pragma once

#include "stm32/device/f4/adc.hpp"
#include "stm32/drivers/adc.hpp"
#include "stm32/hw/adc/f4.hpp"

namespace stm32::drivers::f4 {

using Adc1 = AdcDriver<hw::F4Adc<device::f4::Adc1>>;

} // namespace stm32::drivers::f4
