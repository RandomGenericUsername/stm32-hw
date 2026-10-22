#pragma once

#include <cstddef>
#include <cstdint>

namespace stm32::drivers {

// Type-level UART driver with interrupt-driven TX and RX ring buffers.
//
// HwUart is the stateless hardware facade (WlLpuart, ...). This class owns the
// buffers, the indices and the interrupt bookkeeping. The ISR symbol is emitted
// by the compiled driver source (src/drivers/uart_irq_<family>.cpp), which
// calls on_irq().
//
// Single-producer/single-consumer per direction: the application writes TX
// from thread context, the interrupt drains it; the interrupt fills RX, the
// application reads it. Each index is written by one side only.
template <class HwUart,
          std::size_t TxCapacity = 128,
          std::size_t RxCapacity = TxCapacity>
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
        if (config.receive) {
            HwUart::enable_rx_irq();
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

    // Blocks while the TX ring is full, so no byte is dropped.
    static void write_byte(std::uint8_t value) {
        while (tx_full()) {
        }
        tx_buffer_[tx_head_] = value;
        tx_head_ = next(tx_head_, TxCapacity);
        HwUart::enable_tx_irq();
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
};

} // namespace stm32::drivers
