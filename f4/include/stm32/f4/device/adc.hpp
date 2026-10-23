#pragma once

#include <stm32f4xx.h>

#include "stm32/core/peripheral/peripheral.hpp"

namespace stm32::device::f4 {

// MCU facts only: register block pointers and peripheral identity.
class Adc1 final {
public:
    static constexpr hw::Peripheral id = hw::Peripheral::Adc1;
    static ADC_TypeDef* regs() { return ADC1; }
    static ADC_Common_TypeDef* common() { return ADC1_COMMON; }

private:
    Adc1() = delete;
};

} // namespace stm32::device::f4
