#pragma once

#include <cstdint>

#include <stm32wl55xx.h>

#include "stm32/device/wl55/clock.hpp"
#include "stm32/hw/peripheral.hpp"

namespace stm32::device::wl55 {

// MCU fact: RCC_CCIPR LPUART1SEL / USARTxSEL encodings (RM0453 §7.4.24).
struct UartClockTraits final {
    enum class Source : std::uint32_t {
        Pclk1 = 0,
        Sysclk = 1,
        Hsi16 = 2,
        Lse = 3,
    };
};

// MCU fact only: LPUART1 register block, peripheral identity and IRQ line.
class Lpuart1 final {
public:
    using ClockTraits = device::wl55::ClockTraits;
    using UartClockTraits = device::wl55::UartClockTraits;

    static constexpr hw::Peripheral id = hw::Peripheral::Lpuart1;
    static USART_TypeDef* regs() { return LPUART1; }
    static constexpr IRQn_Type irq() { return LPUART1_IRQn; }

private:
    Lpuart1() = delete;
};

} // namespace stm32::device::wl55
