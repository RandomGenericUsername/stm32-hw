#pragma once

#include <cstdint>

// SysTick-backed delay policy.
//
// Requires the CMSIS device header (SysTick, SystemCoreClock) to be included
// before this file. It uses SysTick as a polled 1 MHz down-counter with the
// interrupt disabled; no SysTick_Handler is required.
//
// This is a policy, not a hard dependency: the ADC accepts any type with
//   static void init();
//   static void wait_us(std::uint32_t);
// so an application that already uses SysTick (or wants a hardware timer /
// DWT / RTOS tick) can inject its own policy.
namespace stm32::hw {

class SysTickDelay {
public:
    SysTickDelay() = delete;
    SysTickDelay(const SysTickDelay&) = delete;
    SysTickDelay& operator=(const SysTickDelay&) = delete;

    static void init() {
        SysTick->LOAD = (SystemCoreClock / 1000000u) - 1u;
        SysTick->VAL = 0;
        // Core clock, enabled, no interrupt.
        SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
    }

    static void wait_us(std::uint32_t us) {
        const std::uint32_t reload = (SystemCoreClock / 1000000u);
        for (std::uint32_t i = 0; i < us; ++i) {
            const std::uint32_t start = SysTick->VAL;
            while (((start - SysTick->VAL) & 0x00FFFFFFu) < reload) {
            }
        }
    }
};

} // namespace stm32::hw
