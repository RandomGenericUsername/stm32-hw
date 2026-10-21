#pragma once

#include "stm32/device/h7/adc.hpp"
#include "stm32/drivers/adc.hpp"
#include "stm32/hw/adc/h7.hpp"

namespace stm32::drivers::h7 {

using Adc1 = AdcDriver<hw::H7Adc<device::h7::Adc1>>;
using Adc2 = AdcDriver<hw::H7Adc<device::h7::Adc2>>;
using Adc3 = AdcDriver<hw::H7Adc<device::h7::Adc3>>;

} // namespace stm32::drivers::h7
