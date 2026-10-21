#pragma once

#include <cstdint>

namespace stm32::hw {

// Coarse busy-wait used for ADC regulator/startup delays.
//
// Requires SystemCoreClock to be declared by the CMSIS system header, which the
// device header includes before the ADC family header includes this file.
inline void delay_us(std::uint32_t us) {
    const std::uint32_t loops =
        (SystemCoreClock / 1000000u) * us;
    for (std::uint32_t i = 0; i < loops; ++i) {
        __asm volatile("nop");
    }
}

} // namespace stm32::hw
