#pragma once

#include <cstdint>

namespace stm32::hw {

// Passive configuration aggregate. The application supplies the board facts:
// baud rate, kernel-clock frequency and kernel-clock source. Traits is the
// family clock traits type; it supplies the source enum encoding.
template <class Traits>
struct UartConfig {
    std::uint32_t baud;
    std::uint32_t kernel_hz;
    typename Traits::Source source;
};

} // namespace stm32::hw
