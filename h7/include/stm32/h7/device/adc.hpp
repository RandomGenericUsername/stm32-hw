#pragma once

#include <stm32h7xx.h>

#include "stm32/core/peripheral/peripheral.hpp"

namespace stm32::device::h7 {

// MCU facts only: register block pointers and peripheral identity.
class Adc1 final {
public:
    static constexpr hw::Peripheral id = hw::Peripheral::Adc1;
    static ADC_TypeDef* regs() { return ADC1; }
    static ADC_Common_TypeDef* common() { return ADC12_COMMON; }

private:
    Adc1() = delete;
};

class Adc2 final {
public:
    static constexpr hw::Peripheral id = hw::Peripheral::Adc2;
    static ADC_TypeDef* regs() { return ADC2; }
    static ADC_Common_TypeDef* common() { return ADC12_COMMON; }

private:
    Adc2() = delete;
};

class Adc3 final {
public:
    static constexpr hw::Peripheral id = hw::Peripheral::Adc3;
    static ADC_TypeDef* regs() { return ADC3; }
    static ADC_Common_TypeDef* common() { return ADC3_COMMON; }

private:
    Adc3() = delete;
};

} // namespace stm32::device::h7
