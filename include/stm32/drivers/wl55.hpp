#pragma once

#include "stm32/device/wl55/adc.hpp"
#include "stm32/drivers/adc.hpp"
#include "stm32/hw/adc/wl55.hpp"

namespace stm32::drivers::wl55 {

class Adc1 final : public AdcDriver<hw::WlAdc<device::wl55::Adc1>> {
public:
    static void enable_vrefint() {
        hw::WlAdc<device::wl55::Adc1>::enable_vrefint();
    }
};

} // namespace stm32::drivers::wl55
