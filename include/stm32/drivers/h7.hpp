#pragma once

#include "stm32/device/h7/adc.hpp"
#include "stm32/drivers/adc.hpp"
#include "stm32/hw/adc/h7.hpp"

namespace stm32::drivers::h7 {

template <class Handler = AdcNullHandler>
using Adc1 = AdcDriver<hw::H7Adc<device::h7::Adc1>, Handler>;

template <class Handler = AdcNullHandler>
using Adc2 = AdcDriver<hw::H7Adc<device::h7::Adc2>, Handler>;

template <class Handler = AdcNullHandler>
using Adc3 = AdcDriver<hw::H7Adc<device::h7::Adc3>, Handler>;

} // namespace stm32::drivers::h7
