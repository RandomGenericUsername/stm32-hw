#pragma once

#include <cstdint>

#include <stm32wl55xx.h>

#include "stm32/device/wl55/clock.hpp"
#include "stm32/hw/peripheral.hpp"

namespace stm32::device::wl55 {

// MCU fact only: register block pointers, peripheral identity, calibration.
class Adc1 final {
public:
    using ClockTraits = device::wl55::ClockTraits;

    static constexpr hw::Peripheral id = hw::Peripheral::Adc1;
    static ADC_TypeDef* regs() { return ADC; }
    static ADC_Common_TypeDef* common() { return ADC_COMMON; }

    // DS13293 §3.20.2, Table 14: 16-bit VREFINT factory calibration value,
    // raw ADC data acquired at VDDA = VREF+ = 3.3 V (±10 mV), 30 °C.
    static constexpr std::uintptr_t vrefint_cal_address = 0x1FFF75AAUL;
    static constexpr std::uint32_t vrefplus_charac_mv = 3300;

private:
    Adc1() = delete;
};

} // namespace stm32::device::wl55
