#pragma once

#include <cstdint>

namespace stm32::hw {

// Temporary shared peripheral identity used during the family-root move.
// The next clock slice splits this into per-family peripheral descriptors.
enum class Peripheral : std::uint16_t {
    GpioA,
    GpioB,
    GpioC,
    GpioH,
    Usart1,
    Lpuart1,
    Adc1,
    Adc2,
    Adc3,
};

} // namespace stm32::hw
