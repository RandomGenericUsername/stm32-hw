#pragma once

#include <cstdint>

namespace stm32::hw {

// Generic single-conversion ADC facade.
//
// Impl supplies:
//   using SampleTime = ...;                 // family-specific 3-bit codes
//   static void init();                     // gate + clock + regulator + cal
//   static void disable();
//   template <std::uint8_t Channel>
//   static void configure(SampleTime);
//   template <std::uint8_t Channel>
//   static std::uint16_t read();            // software start, poll, DR
//
// This first slice is 12-bit, right-aligned, single conversion, polling.
// Resolution, DMA, interrupts, scan/continuous, injected channels, watchdogs,
// oversampling and multimode are deliberately out of scope.
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

    template <std::uint8_t Channel>
    static std::uint16_t read() {
        return Impl::template read<Channel>();
    }
};

} // namespace stm32::hw
