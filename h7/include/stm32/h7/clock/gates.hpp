#pragma once

#include <cstddef>
#include <cstdint>

#include <stm32h7xx.h>

#include "stm32/core/peripheral/peripheral.hpp"
#include "stm32/core/peripheral/clock.hpp"
#include "stm32/core/peripheral/rcc_gate.hpp"

namespace stm32::hw {

// STM32H7 peripheral -> RCC gate mapping.
template <Peripheral P>
class H7Gate;

template <>
class H7Gate<Peripheral::Adc1>
    : public RccGate<
          RCC_BASE,
          offsetof(RCC_TypeDef, AHB1ENR),
          RCC_AHB1ENR_ADC12EN_Pos,
          offsetof(RCC_TypeDef, AHB1RSTR),
          RCC_AHB1RSTR_ADC12RST_Pos> {};

template <>
class H7Gate<Peripheral::Adc2>
    : public RccGate<
          RCC_BASE,
          offsetof(RCC_TypeDef, AHB1ENR),
          RCC_AHB1ENR_ADC12EN_Pos,
          offsetof(RCC_TypeDef, AHB1RSTR),
          RCC_AHB1RSTR_ADC12RST_Pos> {};

template <>
class H7Gate<Peripheral::Adc3>
    : public RccGate<
          RCC_BASE,
          offsetof(RCC_TypeDef, AHB4ENR),
          RCC_AHB4ENR_ADC3EN_Pos,
          offsetof(RCC_TypeDef, AHB4RSTR),
          RCC_AHB4RSTR_ADC3RST_Pos> {};

class H7Clock final : public PeripheralClock<H7Clock> {
public:
    enum class AdcSource : std::uint32_t {
        Pll2 = 0,
        Pll3 = 1,
        ClkP = 2, // per_ck
    };

    H7Clock() = delete;
    H7Clock(const H7Clock&) = delete;
    H7Clock& operator=(const H7Clock&) = delete;

    template <Peripheral P>
    static void enable() {
        H7Gate<P>::enable();
    }

    template <Peripheral P>
    static void disable() {
        H7Gate<P>::disable();
    }

    template <Peripheral P>
    static void reset() {
        H7Gate<P>::reset();
    }

    template <Peripheral P>
    static bool is_enabled() {
        return H7Gate<P>::is_enabled();
    }

    // ADC kernel clock source (RCC_D3CCIPR.ADCSEL). The ADC block's
    // ADC_CCR.PRESC then divides this source.
    static void select_adc_clock(AdcSource source) {
        RCC->D3CCIPR = (RCC->D3CCIPR & ~RCC_D3CCIPR_ADCSEL_Msk)
                     | (static_cast<std::uint32_t>(source)
                        << RCC_D3CCIPR_ADCSEL_Pos);
    }
};

} // namespace stm32::hw
