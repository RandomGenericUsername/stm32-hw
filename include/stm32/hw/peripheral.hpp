#pragma once

#include <cstdint>

namespace stm32::hw {

// Stable peripheral identity used by the clock layer.
// Family clock implementations map these tokens to their own RCC registers.
enum class Peripheral : std::uint16_t {
    GpioA,
    GpioB,
    GpioC,
    GpioH,
    Usart1,
};

} // namespace stm32::hw
