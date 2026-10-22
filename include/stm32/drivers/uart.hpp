#pragma once

#include <cstddef>
#include <cstdint>

namespace stm32::drivers {

// Type-level UART driver with an interrupt-driven TX ring buffer.
//
// HwUart is the stateless hardware facade (WlLpuart, ...). This class owns the
// buffer, the indices and the interrupt bookkeeping. The ISR symbol is emitted
// by the compiled driver source (src/drivers/uart_irq_<family>.cpp), which
// calls on_irq().
//
// Single-producer/single-consumer: the application writes from thread context,
// the interrupt drains. head_ is only written by the producer, tail_ only by
// the consumer; both are volatile. Capacity includes one sentinel slot.
template <class HwUart, std::size_t Capacity = 128>
class UartDriver {
    static_assert(Capacity >= 2, "UART ring buffer needs at least two slots");
    static_assert(Capacity <= 0xFFFFu, "UART ring buffer index is 16-bit");

public:
    static bool init(const typename HwUart::Config& config) {
        head_ = 0;
        tail_ = 0;
        return HwUart::init(config);
    }

    static void stop() {
        HwUart::disable();
        head_ = 0;
        tail_ = 0;
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

    // Blocks while the ring is full, so no byte is dropped.
    static void write_byte(std::uint8_t value) {
        while (full()) {
        }
        buffer_[head_] = value;
        head_ = next(head_);
        HwUart::enable_tx_irq();
    }

    // Wait until every queued byte has left the shift register.
    static void flush() {
        while (!empty()) {
        }
        while (!HwUart::tx_complete()) {
        }
    }

    static bool empty() {
        return head_ == tail_;
    }

    static std::size_t pending() {
        return static_cast<std::size_t>(
            (static_cast<std::uint32_t>(head_) + Capacity - tail_) % Capacity);
    }

    static void set_irq_priority(std::uint32_t priority) {
        HwUart::set_irq_priority(priority);
    }

    // Called by the library-owned vector. Returns true when the interrupt
    // belonged to this UART.
    static bool on_irq() {
        if (!HwUart::tx_irq_pending()) {
            return false;
        }
        if (empty()) {
            HwUart::disable_tx_irq();
            return true;
        }
        HwUart::write_byte(buffer_[tail_]);
        tail_ = next(tail_);
        return true;
    }

private:
    static constexpr std::uint16_t next(std::uint16_t index) {
        return static_cast<std::uint16_t>((index + 1u) % Capacity);
    }

    static bool full() {
        return next(head_) == tail_;
    }

    static inline volatile std::uint16_t head_ = 0;
    static inline volatile std::uint16_t tail_ = 0;
    static inline volatile std::uint8_t buffer_[Capacity] = {};
};

} // namespace stm32::drivers
