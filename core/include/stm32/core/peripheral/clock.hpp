#pragma once

#include "stm32/core/peripheral/peripheral.hpp"

namespace stm32::hw {

// Abstract peripheral-clock interface.
//
// This is a type-level singleton: no clock object is ever constructed. The
// concrete implementation (WlClock, F4Clock, ...) supplies the static methods.
// Temporary home during the family-root move; the next slice replaces this
// with per-family clock gates.
template <class Impl>
class PeripheralClock {
public:
    PeripheralClock() = delete;
    PeripheralClock(const PeripheralClock&) = delete;
    PeripheralClock& operator=(const PeripheralClock&) = delete;

    template <Peripheral P>
    static void enable() {
        Impl::template enable<P>();
    }

    template <Peripheral P>
    static void disable() {
        Impl::template disable<P>();
    }

    template <Peripheral P>
    static void reset() {
        Impl::template reset<P>();
    }

    template <Peripheral P>
    static bool is_enabled() {
        return Impl::template is_enabled<P>();
    }
};

} // namespace stm32::hw
