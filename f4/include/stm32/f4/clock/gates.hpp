#pragma once

#include <cstddef>
#include <cstdint>

#include <stm32f4xx.h>

#include "stm32/core/peripheral/peripheral.hpp"
#include "stm32/core/peripheral/clock.hpp"
#include "stm32/core/peripheral/rcc_gate.hpp"

namespace stm32::hw {

// STM32F4 peripheral -> RCC gate mapping.
template <Peripheral P>
class F4Gate;

template <>
class F4Gate<Peripheral::Adc1>
    : public RccGate<
          RCC_BASE,
          offsetof(RCC_TypeDef, APB2ENR),
          RCC_APB2ENR_ADC1EN_Pos,
          offsetof(RCC_TypeDef, APB2RSTR),
          RCC_APB2RSTR_ADCRST_Pos> {};

class F4Clock final : public PeripheralClock<F4Clock> {
public:
    F4Clock() = delete;
    F4Clock(const F4Clock&) = delete;
    F4Clock& operator=(const F4Clock&) = delete;

    template <Peripheral P>
    static void enable() {
        F4Gate<P>::enable();
    }

    template <Peripheral P>
    static void disable() {
        F4Gate<P>::disable();
    }

    template <Peripheral P>
    static void reset() {
        F4Gate<P>::reset();
    }

    template <Peripheral P>
    static bool is_enabled() {
        return F4Gate<P>::is_enabled();
    }
};

} // namespace stm32::hw
