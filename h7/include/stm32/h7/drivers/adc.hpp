#pragma once

#include "stm32/h7/device/adc.hpp"
#include "stm32/core/drivers/adc.hpp"
#include "stm32/h7/adc/hardware.hpp"

namespace stm32::drivers::h7 {

template <AdcHandler Handler = AdcNullHandler>
using Adc1 = AdcDriver<hw::H7Adc<device::h7::Adc1>, Handler>;

template <AdcHandler Handler = AdcNullHandler>
using Adc2 = AdcDriver<hw::H7Adc<device::h7::Adc2>, Handler>;

template <AdcHandler Handler = AdcNullHandler>
using Adc3 = AdcDriver<hw::H7Adc<device::h7::Adc3>, Handler>;

} // namespace stm32::drivers::h7
