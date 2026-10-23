#pragma once

#include <cstdint>

namespace stm32::hw {

// Generic ADC facade.
//
// Impl supplies:
//   using SampleTime = ...;                 // family-specific 3-bit codes
//   static void init();                     // gate + clock + regulator + cal
//   static void disable();
//   template <std::uint8_t Channel>
//   static void configure(SampleTime);
//   static void start();                    // enable EOC IRQ + SW trigger
//   static bool irq_pending();
//   static std::uint16_t read_result();     // clear EOC, return DR
//   static void enable_irq();
//   static void disable_irq();
//   static void set_irq_priority(std::uint32_t);
//   template <std::uint8_t Channel>
//   static std::uint16_t read();            // blocking convenience
//
// The interrupt path is the primary one: configure + start, then
// handle the IRQ in the application and call read_result(). read() is kept as
// an explicit blocking convenience for init/selftest.
template <class Impl>
class Adc {
public:
    Adc() = delete;
    Adc(const Adc&) = delete;
    Adc& operator=(const Adc&) = delete;

    static void init() {
        Impl::init();
    }

    static void disable() {
        Impl::disable();
    }

    template <std::uint8_t Channel, class SampleTime>
    static void configure(SampleTime sample_time) {
        Impl::template configure<Channel>(sample_time);
    }

    static void start() {
        Impl::start();
    }

    static bool irq_pending() {
        return Impl::irq_pending();
    }

    static std::uint16_t read_result() {
        return Impl::read_result();
    }

    static void enable_irq() {
        Impl::enable_irq();
    }

    static void disable_irq() {
        Impl::disable_irq();
    }

    static void set_irq_priority(std::uint32_t priority) {
        Impl::set_irq_priority(priority);
    }

    template <std::uint8_t Channel>
    static std::uint16_t read() {
        return Impl::template read<Channel>();
    }
};

} // namespace stm32::hw
