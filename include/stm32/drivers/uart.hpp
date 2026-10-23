#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>

#include "stm32/drivers/uart_irq.hpp"

namespace stm32::drivers {

// Default handler: no application event.
struct UartNullHandler {
    static constexpr bool wants_idle = false;
    void on_byte(std::uint8_t) {}
    void on_idle() {}
};

// UART handler contract. on_byte(byte) is always required. on_idle() is
// required only when the handler asks for IDLE interrupts.
template <class H>
concept UartIdleHandler = requires(H& handler) {
    { handler.on_idle() } -> std::same_as<void>;
};

template <class H>
concept UartHandler =
    std::default_initializable<H> &&
    requires {
        { H::wants_idle } -> std::convertible_to<bool>;
    } &&
    requires(H& handler, std::uint8_t byte) {
        { handler.on_byte(byte) } -> std::same_as<void>;
    } &&
    (!H::wants_idle || UartIdleHandler<H>);

// Type-level UART driver with interrupt-driven TX and RX ring buffers.
//
// HwUart is the stateless hardware facade (WlLpuart, ...). Handler is a
// compile-time policy whose on_byte(byte) runs for every RX byte and whose
// on_idle() runs on a USART IDLE event when wants_idle is true. The driver
// owns the handler object by value: no callback pointer, no context lifetime
// problem. The ISR symbol is emitted by the compiled driver source
// (src/drivers/uart_irq_<family>.cpp), which calls the router.
//
// Single-producer/single-consumer per direction: the application writes TX
// from thread context, the interrupt drains it; the interrupt fills RX, the
// application reads it. Each index is written by one side only.
template <class HwUart,
          std::size_t TxCapacity = 128,
          std::size_t RxCapacity = TxCapacity,
          UartHandler Handler = UartNullHandler>
class UartDriver {
    static_assert(TxCapacity >= 2, "UART TX ring needs at least two slots");
    static_assert(RxCapacity >= 2, "UART RX ring needs at least two slots");
    static_assert(TxCapacity <= 0xFFFFu, "UART TX index is 16-bit");
    static_assert(RxCapacity <= 0xFFFFu, "UART RX index is 16-bit");

public:
    static bool init(const typename HwUart::Config& config) {
        tx_head_ = 0;
        tx_tail_ = 0;
        rx_head_ = 0;
        rx_tail_ = 0;
        rx_overrun_ = false;

        if (!HwUart::init(config)) {
            return false;
        }

        UartIrqRouter<HwUart::irq()>::attach(&on_irq);

        if (config.receive) {
            HwUart::enable_rx_irq();
        }
        if constexpr (Handler::wants_idle) {
            HwUart::enable_idle_irq();
        }
        return true;
    }

    static void stop() {
        HwUart::disable();
        tx_head_ = 0;
        tx_tail_ = 0;
        rx_head_ = 0;
        rx_tail_ = 0;
    }

    static void write(const char* text) {
        while (*text != '\0') {
            write_byte(static_cast<std::uint8_t>(*text));
            ++text;
        }
    }

    static void write(const std::uint8_t* data, std::size_t size) {
        for (std::size_t i = 0; i < size; ++i) {
            write_byte(data[i]);
        }
    }

    // Non-blocking TX helpers, safe to call from an ISR or callback.
    static bool try_write(const char* text) {
        while (*text != '\0') {
            if (!try_write_byte(static_cast<std::uint8_t>(*text))) {
                return false;
            }
            ++text;
        }
        return true;
    }

    static bool try_write(const std::uint8_t* data, std::size_t size) {
        for (std::size_t i = 0; i < size; ++i) {
            if (!try_write_byte(data[i])) {
                return false;
            }
        }
        return true;
    }

    static bool try_write_byte(std::uint8_t value) {
        if (tx_full()) {
            return false;
        }
        tx_buffer_[tx_head_] = value;
        tx_head_ = next(tx_head_, TxCapacity);
        HwUart::enable_tx_irq();
        return true;
    }

    // Blocks while the TX ring is full, so no byte is dropped.
    static void write_byte(std::uint8_t value) {
        while (!try_write_byte(value)) {
        }
    }

    // Wait until every queued TX byte has left the shift register.
    static void flush() {
        while (!tx_empty()) {
        }
        while (!HwUart::tx_complete()) {
        }
    }

    static bool empty() {
        return tx_empty();
    }

    static std::size_t pending() {
        return static_cast<std::size_t>(
            (static_cast<std::uint32_t>(tx_head_) + TxCapacity - tx_tail_)
            % TxCapacity);
    }

    // Non-blocking RX read. Returns false when the RX ring is empty.
    static bool read(std::uint8_t& value) {
        if (rx_empty()) {
            return false;
        }
        value = rx_buffer_[rx_tail_];
        rx_tail_ = next(rx_tail_, RxCapacity);
        return true;
    }

    static bool rx_empty() {
        return rx_head_ == rx_tail_;
    }

    static std::size_t rx_pending() {
        return static_cast<std::size_t>(
            (static_cast<std::uint32_t>(rx_head_) + RxCapacity - rx_tail_)
            % RxCapacity);
    }

    static bool rx_overrun() {
        return rx_overrun_;
    }

    static void clear_rx_overrun() {
        rx_overrun_ = false;
    }

    // The handler is owned by the driver. Its lifetime is the driver's.
    static Handler& handler() {
        return handler_;
    }

    static void set_irq_priority(std::uint32_t priority) {
        HwUart::set_irq_priority(priority);
    }

    // Called by the library-owned vector. Returns true when the interrupt
    // belonged to this UART. Handles both directions in one entry.
    static bool on_irq() {
        bool handled = false;

        if (HwUart::rx_irq_pending()) {
            handled = true;
            const std::uint8_t value = HwUart::read_byte();
            if (!rx_full()) {
                rx_buffer_[rx_head_] = value;
                rx_head_ = next(rx_head_, RxCapacity);
            } else {
                // Drop the byte but still read RDR above to clear RXNE.
                rx_overrun_ = true;
            }
            handler_.on_byte(value);
        }

        if (HwUart::idle_pending()) {
            handled = true;
            HwUart::clear_idle();
            if constexpr (Handler::wants_idle) {
                handler_.on_idle();
            }
        }

        if (HwUart::tx_irq_pending()) {
            handled = true;
            if (tx_empty()) {
                HwUart::disable_tx_irq();
            } else {
                HwUart::write_byte(tx_buffer_[tx_tail_]);
                tx_tail_ = next(tx_tail_, TxCapacity);
            }
        }

        return handled;
    }

private:
    static constexpr std::uint16_t next(std::uint16_t index,
                                        std::size_t capacity) {
        return static_cast<std::uint16_t>((index + 1u) % capacity);
    }

    static bool tx_empty() {
        return tx_head_ == tx_tail_;
    }

    static bool tx_full() {
        return next(tx_head_, TxCapacity) == tx_tail_;
    }

    static bool rx_full() {
        return next(rx_head_, RxCapacity) == rx_tail_;
    }

    static inline volatile std::uint16_t tx_head_ = 0;
    static inline volatile std::uint16_t tx_tail_ = 0;
    static inline volatile std::uint16_t rx_head_ = 0;
    static inline volatile std::uint16_t rx_tail_ = 0;
    static inline volatile bool rx_overrun_ = false;
    static inline volatile std::uint8_t tx_buffer_[TxCapacity] = {};
    static inline volatile std::uint8_t rx_buffer_[RxCapacity] = {};
    static inline Handler handler_{};
};

} // namespace stm32::drivers
