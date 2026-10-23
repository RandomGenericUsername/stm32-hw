#pragma once

#include <cstdint>

#include "stm32/core/gpio/port.hpp"

namespace stm32::hw {

// Stateless pin facade. Pin is a type-level singleton, not an object.
//
// PortDesc supplies the port identity and CMSIS register pointer:
//   static constexpr Peripheral id;
//   static GPIO_TypeDef* regs();
//
// Clock supplies:
//   template <Peripheral P> static void enable();
template <class PortDesc, std::uint8_t Number, class Clock>
class Pin {
    static_assert(Number < 16, "GPIO pin number must be in 0..15");

public:
    Pin() = delete;
    Pin(const Pin&) = delete;
    Pin& operator=(const Pin&) = delete;

    static void init(const GpioConfig& cfg) {
        Clock::template enable<PortDesc::id>();
        Port<PortDesc>::configure(Number, cfg);
    }

    static void set() {
        Port<PortDesc>::set(1u << Number);
    }

    static void reset() {
        Port<PortDesc>::reset(1u << Number);
    }

    static void toggle() {
        Port<PortDesc>::toggle(1u << Number);
    }

    static bool read() {
        return (Port<PortDesc>::read() & (1u << Number)) != 0;
    }
};

} // namespace stm32::hw
