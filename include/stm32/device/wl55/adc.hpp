#pragma once

#include <stm32wl55xx.h>

#include "stm32/hw/peripheral.hpp"

namespace stm32::device::wl55 {

// MCU fact only: register block pointers and peripheral identity.
class Adc1 final {
public:
    static constexpr hw::Peripheral id = hw::Peripheral::Adc1;
    static ADC_TypeDef* regs() { return ADC; }
    static ADC_Common_TypeDef* common() { return ADC_COMMON; }

private:
    Adc1() = delete;
};

} // namespace stm32::device::wl55
