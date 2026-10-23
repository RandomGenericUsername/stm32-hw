#pragma once

#include <cstdint>

#include "stm32/core/uart/config.hpp"

namespace stm32::hw {

// Generic UART facade.
//
// Traits supplies the family kernel-clock source enum and therefore the
// Config type. Impl supplies the mechanisms:
//   static bool init(const Config&);       // gate + kernel mux + baud
//   static void disable();
//   static void write_byte(std::uint8_t);
//   static bool tx_ready();
//   static bool tx_complete();
//   static void enable_tx_irq();
//   static void disable_tx_irq();
//   static bool tx_irq_pending();
//   static std::uint8_t read_byte();
//   static bool rx_ready();
//   static void enable_rx_irq();
//   static void disable_rx_irq();
//   static bool rx_irq_pending();
//   static bool idle_pending();
//   static void clear_idle();
//   static void enable_idle_irq();
//   static void disable_idle_irq();
//   static void set_irq_priority(std::uint32_t);
//   static constexpr auto irq();
//
// init() returns false when the requested baud rate cannot be represented at
// the given kernel frequency.
template <class Impl, class Traits>
class Uart {
public:
    using Config = UartConfig<Traits>;

    Uart() = delete;
    Uart(const Uart&) = delete;
    Uart& operator=(const Uart&) = delete;

    static bool init(const Config& config) {
        return Impl::init(config);
    }

    static void disable() {
        Impl::disable();
    }

    static void write_byte(std::uint8_t value) {
        Impl::write_byte(value);
    }

    static bool tx_ready() {
        return Impl::tx_ready();
    }

    static bool tx_complete() {
        return Impl::tx_complete();
    }

    static void enable_tx_irq() {
        Impl::enable_tx_irq();
    }

    static void disable_tx_irq() {
        Impl::disable_tx_irq();
    }

    static bool tx_irq_pending() {
        return Impl::tx_irq_pending();
    }

    static std::uint8_t read_byte() {
        return Impl::read_byte();
    }

    static bool rx_ready() {
        return Impl::rx_ready();
    }

    static void enable_rx_irq() {
        Impl::enable_rx_irq();
    }

    static void disable_rx_irq() {
        Impl::disable_rx_irq();
    }

    static bool rx_irq_pending() {
        return Impl::rx_irq_pending();
    }

    static bool idle_pending() {
        return Impl::idle_pending();
    }

    static void clear_idle() {
        Impl::clear_idle();
    }

    static void enable_idle_irq() {
        Impl::enable_idle_irq();
    }

    static void disable_idle_irq() {
        Impl::disable_idle_irq();
    }

    static void set_irq_priority(std::uint32_t priority) {
        Impl::set_irq_priority(priority);
    }

    static constexpr auto irq() {
        return Impl::irq();
    }
};

} // namespace stm32::hw
