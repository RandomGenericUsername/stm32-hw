#pragma once

#include <cstddef>
#include <cstdint>

#include <stm32wl55xx.h>

#include "stm32/hw/peripheral.hpp"
#include "stm32/hw/peripheral_clock.hpp"
#include "stm32/hw/rcc_gate.hpp"

namespace stm32::hw {

// WL55 peripheral -> RCC gate mapping.
//
// The primary template is intentionally undefined: using a peripheral that the
// family does not provide is a compile-time error.
template <Peripheral P>
class WlGate;

template <>
class WlGate<Peripheral::GpioA>
    : public RccGate<
          RCC_BASE,
          offsetof(RCC_TypeDef, AHB2ENR),
          RCC_AHB2ENR_GPIOAEN_Pos,
          offsetof(RCC_TypeDef, AHB2RSTR),
          RCC_AHB2RSTR_GPIOARST_Pos> {};

template <>
class WlGate<Peripheral::GpioB>
    : public RccGate<
          RCC_BASE,
          offsetof(RCC_TypeDef, AHB2ENR),
          RCC_AHB2ENR_GPIOBEN_Pos,
          offsetof(RCC_TypeDef, AHB2RSTR),
          RCC_AHB2RSTR_GPIOBRST_Pos> {};

template <>
class WlGate<Peripheral::GpioC>
    : public RccGate<
          RCC_BASE,
          offsetof(RCC_TypeDef, AHB2ENR),
          RCC_AHB2ENR_GPIOCEN_Pos,
          offsetof(RCC_TypeDef, AHB2RSTR),
          RCC_AHB2RSTR_GPIOCRST_Pos> {};

template <>
class WlGate<Peripheral::GpioH>
    : public RccGate<
          RCC_BASE,
          offsetof(RCC_TypeDef, AHB2ENR),
          RCC_AHB2ENR_GPIOHEN_Pos,
          offsetof(RCC_TypeDef, AHB2RSTR),
          RCC_AHB2RSTR_GPIOHRST_Pos> {};

template <>
class WlGate<Peripheral::Usart1>
    : public RccGate<
          RCC_BASE,
          offsetof(RCC_TypeDef, APB2ENR),
          RCC_APB2ENR_USART1EN_Pos,
          offsetof(RCC_TypeDef, APB2RSTR),
          RCC_APB2RSTR_USART1RST_Pos> {};

template <>
class WlGate<Peripheral::Adc1>
    : public RccGate<
          RCC_BASE,
          offsetof(RCC_TypeDef, APB2ENR),
          RCC_APB2ENR_ADCEN_Pos,
          offsetof(RCC_TypeDef, APB2RSTR),
          RCC_APB2RSTR_ADCRST_Pos> {};

// WL55 clock implementation (type-level singleton).
class WlClock final : public PeripheralClock<WlClock> {
public:
    enum class AdcSource : std::uint32_t {
        None = 0,
        Hsi16 = 1,
        PllP = 2,
        Sysclk = 3,
    };

    WlClock() = delete;
    WlClock(const WlClock&) = delete;
    WlClock& operator=(const WlClock&) = delete;

    template <Peripheral P>
    static void enable() {
        WlGate<P>::enable();
    }

    template <Peripheral P>
    static void disable() {
        WlGate<P>::disable();
    }

    template <Peripheral P>
    static void reset() {
        WlGate<P>::reset();
    }

    template <Peripheral P>
    static bool is_enabled() {
        return WlGate<P>::is_enabled();
    }

    // ADC kernel clock source (RCC_CCIPR.ADCSEL). The ADC block's
    // ADC_CCR.PRESC / CFGR2.CKMODE then scale/route this source.
    static void select_adc_clock(AdcSource source) {
        if (source == AdcSource::Hsi16) {
            // Reset leaves HSI16 off while the system runs from MSI. The ADC
            // cannot calibrate or convert until its selected kernel clock is
            // actually running.
            RCC->CR |= RCC_CR_HSION;
            while ((RCC->CR & RCC_CR_HSIRDY) == 0u) {
            }
        }

        RCC->CCIPR = (RCC->CCIPR & ~RCC_CCIPR_ADCSEL_Msk)
                   | (static_cast<std::uint32_t>(source)
                      << RCC_CCIPR_ADCSEL_Pos);
    }
};

} // namespace stm32::hw
