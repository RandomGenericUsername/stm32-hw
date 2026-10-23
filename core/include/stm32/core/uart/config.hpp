#pragma once

#include <cstdint>

#include "stm32/core/clock/config.hpp"

namespace stm32::hw {

// Passive configuration aggregate. The application supplies the board facts:
// baud rate, kernel-clock frequency and kernel-clock source. Traits is the
// family clock traits type; it supplies the source enum encoding.
template <class Traits>
struct UartConfig {
    using Source = typename Traits::Source;

    std::uint32_t baud;
    std::uint32_t kernel_hz;
    Source source;
    bool receive = true;

    // Derive the kernel frequency from the selected system clock tree instead
    // of duplicating it in the application. LSE is a board fact and must be
    // supplied explicitly when selected.
    template <class ClockTraits>
    static constexpr UartConfig from_system(
        const BasicSystemClockConfig<ClockTraits>& sys,
        std::uint32_t baud,
        Source source,
        std::uint32_t lse_hz = 0u,
        bool receive = true) {
        std::uint32_t hz = 0u;
        switch (source) {
        case Source::Pclk1:
            hz = sys.pclk1_hz();
            break;
        case Source::Sysclk:
            hz = sys.sysclk_hz();
            break;
        case Source::Hsi16:
            hz = ClockTraits::hsi16_hz;
            break;
        case Source::Lse:
            hz = lse_hz;
            break;
        }
        return UartConfig{baud, hz, source, receive};
    }
};

} // namespace stm32::hw
