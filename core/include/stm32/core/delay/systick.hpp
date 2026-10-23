#pragma once

#include <cstdint>

// SysTick-backed delay policy.
//
// Requires the CMSIS device header (SysTick, SystemCoreClock) to be included
// before this file. It uses SysTick as a polled free-running 24-bit down
// counter at the core clock, with the interrupt disabled; no SysTick_Handler
// is required. wait_us() converts microseconds to core ticks.
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
        // Free-running 24-bit counter. Do not use a 1 MHz reload here: on a
        // low core clock (for example 4 MHz) the reload is only 4 ticks, and
        // the 24-bit difference wraps back to 0 before it can reach the
        // reload, which makes the wait loop spin forever.
        SysTick->LOAD = 0x00FFFFFFu;
        SysTick->VAL = 0;
        // Core clock, enabled, no interrupt.
        SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
    }

    static void wait_us(std::uint32_t us) {
        const std::uint32_t ticks_per_us = SystemCoreClock / 1000000u;
        if (ticks_per_us == 0u) {
            return;
        }

        // Keep each chunk below the 24-bit counter period.
        const std::uint32_t max_us = 0x00FFFFFFu / ticks_per_us;
        while (us != 0u) {
            const std::uint32_t chunk_us = (us > max_us) ? max_us : us;
            const std::uint32_t ticks = chunk_us * ticks_per_us;
            const std::uint32_t start = SysTick->VAL;
            while (((start - SysTick->VAL) & 0x00FFFFFFu) < ticks) {
            }
            us -= chunk_us;
        }
    }
};

} // namespace stm32::hw
