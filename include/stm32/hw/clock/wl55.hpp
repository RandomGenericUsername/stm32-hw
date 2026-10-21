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

    // RCC_CFGR.SW / SWS encodings (RM0453 §7.4.3).
    enum class SysclkSource : std::uint32_t {
        Msi = 0,
        Hsi16 = 1,
        Hse32 = 2,
        Pll = 3,
    };

    // RCC_CFGR.HPRE encodings.
    enum class AhbDivider : std::uint32_t {
        Div1 = 0,
        Div2 = 8,
        Div4 = 9,
        Div8 = 10,
        Div16 = 11,
        Div64 = 12,
        Div128 = 13,
        Div256 = 14,
        Div512 = 15,
    };

    // RCC_CFGR.PPRE1/PPRE2 encodings.
    enum class ApbDivider : std::uint32_t {
        Div1 = 0,
        Div2 = 4,
        Div4 = 5,
        Div8 = 6,
        Div16 = 7,
    };

    static constexpr std::uint32_t hsi16_hz = 16000000u;
    static constexpr std::uint32_t msi_reset_hz = 4000000u;

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

    static void enable_hsi16() {
        RCC->CR |= RCC_CR_HSION;
        while ((RCC->CR & RCC_CR_HSIRDY) == 0u) {
        }
    }

    // VCORE range 1 is the high-performance range. Reset is already range 1;
    // this only changes it when a previous configuration selected range 2.
    static void ensure_voltage_range1() {
        const std::uint32_t range1 = 1u << PWR_CR1_VOS_Pos;
        if ((PWR->CR1 & PWR_CR1_VOS_Msk) != range1) {
            PWR->CR1 = (PWR->CR1 & ~PWR_CR1_VOS_Msk) | range1;
            while ((PWR->SR2 & PWR_SR2_VOSF) != 0u) {
            }
        }
    }

    // RM0453 Table 12: range 1 needs 0 WS up to 18 MHz, 1 WS up to 36 MHz,
    // 2 WS up to 48 MHz.
    static void set_flash_latency(std::uint32_t wait_states) {
        const std::uint32_t bits =
            (wait_states << FLASH_ACR_LATENCY_Pos) & FLASH_ACR_LATENCY_Msk;
        FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY_Msk) | bits;
        while ((FLASH->ACR & FLASH_ACR_LATENCY_Msk) != bits) {
        }
    }

    static void set_ahb_divider(AhbDivider divider) {
        const std::uint32_t bits = (static_cast<std::uint32_t>(divider)
                                    << RCC_CFGR_HPRE_Pos)
                                 & RCC_CFGR_HPRE_Msk;
        RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_HPRE_Msk) | bits;
        while ((RCC->CFGR & RCC_CFGR_HPRE_Msk) != bits) {
        }
    }

    static void set_apb1_divider(ApbDivider divider) {
        const std::uint32_t bits = (static_cast<std::uint32_t>(divider)
                                    << RCC_CFGR_PPRE1_Pos)
                                 & RCC_CFGR_PPRE1_Msk;
        RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_PPRE1_Msk) | bits;
        while ((RCC->CFGR & RCC_CFGR_PPRE1_Msk) != bits) {
        }
    }

    static void set_apb2_divider(ApbDivider divider) {
        const std::uint32_t bits = (static_cast<std::uint32_t>(divider)
                                    << RCC_CFGR_PPRE2_Pos)
                                 & RCC_CFGR_PPRE2_Msk;
        RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_PPRE2_Msk) | bits;
        while ((RCC->CFGR & RCC_CFGR_PPRE2_Msk) != bits) {
        }
    }

    static void select_sysclk(SysclkSource source) {
        const std::uint32_t sw = (static_cast<std::uint32_t>(source)
                                  << RCC_CFGR_SW_Pos)
                               & RCC_CFGR_SW_Msk;
        const std::uint32_t sws = (static_cast<std::uint32_t>(source)
                                   << RCC_CFGR_SWS_Pos)
                                & RCC_CFGR_SWS_Msk;

        RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_Msk) | sw;
        while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != sws) {
        }
    }

    // Reset-time clock tree: MSI 4 MHz -> HSI16 16 MHz, AHB/APB1/APB2 /1,
    // range 1, 0 flash wait states. Call before enabling SysTick-based delays.
    // Result: HCLK = PCLK1 = PCLK2 = SysTick = 16 MHz.
    static void use_hsi16_16mhz() {
        enable_hsi16();
        ensure_voltage_range1();
        set_flash_latency(0);
        set_ahb_divider(AhbDivider::Div1);
        set_apb1_divider(ApbDivider::Div1);
        set_apb2_divider(ApbDivider::Div1);
        select_sysclk(SysclkSource::Hsi16);
        SystemCoreClockUpdate();
    }

    // ADC kernel clock source (RCC_CCIPR.ADCSEL). The ADC block's
    // ADC_CCR.PRESC / CFGR2.CKMODE then scale/route this source.
    static void select_adc_clock(AdcSource source) {
        if (source == AdcSource::Hsi16) {
            // Reset leaves HSI16 off while the system runs from MSI. The ADC
            // cannot calibrate or convert until its selected kernel clock is
            // actually running.
            enable_hsi16();
        }

        RCC->CCIPR = (RCC->CCIPR & ~RCC_CCIPR_ADCSEL_Msk)
                   | (static_cast<std::uint32_t>(source)
                      << RCC_CCIPR_ADCSEL_Pos);
    }
};

} // namespace stm32::hw
