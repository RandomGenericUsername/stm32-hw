#pragma once

#include <cstddef>
#include <cstdint>

#include <stm32wl55xx.h>

#include "stm32/core/clock/config.hpp"
#include "stm32/core/peripheral/peripheral.hpp"
#include "stm32/core/peripheral/clock.hpp"
#include "stm32/core/peripheral/rcc_gate.hpp"

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
class WlGate<Peripheral::Lpuart1>
    : public RccGate<
          RCC_BASE,
          offsetof(RCC_TypeDef, APB1ENR2),
          RCC_APB1ENR2_LPUART1EN_Pos,
          offsetof(RCC_TypeDef, APB1RSTR2),
          RCC_APB1RSTR2_LPUART1RST_Pos> {};

template <>
class WlGate<Peripheral::Adc1>
    : public RccGate<
          RCC_BASE,
          offsetof(RCC_TypeDef, APB2ENR),
          RCC_APB2ENR_ADCEN_Pos,
          offsetof(RCC_TypeDef, APB2RSTR),
          RCC_APB2RSTR_ADCRST_Pos> {};

// WL55 RCC implementation, parameterized by a board/device clock traits type.
template <class Traits>
class WlClock final : public PeripheralClock<WlClock<Traits>> {
public:
    using Config = BasicSystemClockConfig<Traits>;
    using Source = typename Traits::Source;
    using SourceKind = typename Source::Kind;
    using MsiRange = typename Traits::MsiRange;
    using PllConfig = typename Traits::PllConfig;
    using PllInput = typename PllConfig::Input;

    // RCC_CFGR.SW / SWS encodings (RM0453 §7.4.3).
    enum class SysclkSource : std::uint32_t {
        Msi = 0,
        Hsi16 = 1,
        Hse32 = 2,
        Pll = 3,
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

    // Apply a declarative system clock tree. Intended for reset-time
    // configuration; callers must static_assert(config.valid()).
    static void configure(const Config& config) {
        ensure_voltage_range1();

        const std::uint32_t new_hclk = config.hclk_hz();
        const std::uint32_t old_hclk = SystemCoreClock;

        if (new_hclk > old_hclk) {
            set_flash_latency(config.flash_wait_states());
        }

        prepare_source(config.source);

        set_ahb_divider(config.ahb);
        set_apb1_divider(config.apb1);
        set_apb2_divider(config.apb2);

        select_sysclk(sysclk_source(config.source.kind));

        if (new_hclk <= old_hclk) {
            set_flash_latency(config.flash_wait_states());
        }

        SystemCoreClockUpdate();
    }

    static void enable_hsi16() {
        RCC->CR |= RCC_CR_HSION;
        while ((RCC->CR & RCC_CR_HSIRDY) == 0u) {
        }
    }

    // Write RCC_CCIPR.ADCSEL. The ADC layer owns the source enum and ensures
    // the selected source is running; this function only performs the mux
    // write, because ADCSEL is an RCC register.
    static void select_adc_clock(std::uint32_t source_code) {
        RCC->CCIPR = (RCC->CCIPR & ~RCC_CCIPR_ADCSEL_Msk)
                   | ((source_code << RCC_CCIPR_ADCSEL_Pos)
                      & RCC_CCIPR_ADCSEL_Msk);
    }

    // Write RCC_CCIPR.LPUART1SEL. The UART layer owns the source enum and the
    // baud clock frequency; this only performs the mux write.
    static void select_lpuart1_clock(std::uint32_t source_code) {
        RCC->CCIPR = (RCC->CCIPR & ~RCC_CCIPR_LPUART1SEL_Msk)
                   | ((source_code << RCC_CCIPR_LPUART1SEL_Pos)
                      & RCC_CCIPR_LPUART1SEL_Msk);
    }

private:
    static constexpr SysclkSource sysclk_source(SourceKind kind) {
        switch (kind) {
        case SourceKind::Msi: return SysclkSource::Msi;
        case SourceKind::Hsi16: return SysclkSource::Hsi16;
        case SourceKind::Hse32: return SysclkSource::Hse32;
        case SourceKind::Pll: return SysclkSource::Pll;
        }
        return SysclkSource::Msi;
    }

    static void prepare_source(const Source& source) {
        switch (source.kind) {
        case SourceKind::Msi:
            enable_msi(source.msi_range);
            break;
        case SourceKind::Hsi16:
            enable_hsi16();
            break;
        case SourceKind::Hse32:
            enable_hse32(source.hse_div2);
            break;
        case SourceKind::Pll:
            prepare_pll_input(source.pll_config);
            configure_pll(source.pll_config);
            break;
        }
    }

    static void prepare_pll_input(const PllConfig& pll) {
        switch (pll.input) {
        case PllInput::Msi:
            enable_msi(pll.msi);
            break;
        case PllInput::Hsi16:
            enable_hsi16();
            break;
        case PllInput::Hse32:
            enable_hse32(false);
            break;
        }
    }

    static void enable_msi(MsiRange range) {
        RCC->CR |= RCC_CR_MSION;
        while ((RCC->CR & RCC_CR_MSIRDY) == 0u) {
        }

        const std::uint32_t code = Traits::msi_code(range);
        RCC->CR = (RCC->CR
                   & ~(RCC_CR_MSIRANGE_Msk | RCC_CR_MSIRGSEL))
                | RCC_CR_MSIRGSEL
                | (code << RCC_CR_MSIRANGE_Pos);
        while ((RCC->CR & RCC_CR_MSIRDY) == 0u) {
        }
    }

    static void enable_hse32(bool divide_by_2) {
        if (Traits::hse_tcxo) {
            RCC->CR |= RCC_CR_HSEBYPPWR; // power the TCXO regulator
        } else {
            RCC->CR &= ~RCC_CR_HSEBYPPWR;
        }
        RCC->CR |= RCC_CR_HSEON;
        while ((RCC->CR & RCC_CR_HSERDY) == 0u) {
        }
        if (divide_by_2) {
            RCC->CR |= RCC_CR_HSEPRE;
        } else {
            RCC->CR &= ~RCC_CR_HSEPRE;
        }
    }

    static void configure_pll(const PllConfig& pll) {
        RCC->CR &= ~RCC_CR_PLLON;
        while ((RCC->CR & RCC_CR_PLLRDY) != 0u) {
        }

        std::uint32_t pllcfgr = 0;
        pllcfgr |= (pll_source_code(pll.input)
                    << RCC_PLLCFGR_PLLSRC_Pos);
        pllcfgr |= ((pll.m - 1u) << RCC_PLLCFGR_PLLM_Pos);
        pllcfgr |= (pll.n << RCC_PLLCFGR_PLLN_Pos);
        pllcfgr |= ((pll.r - 1u) << RCC_PLLCFGR_PLLR_Pos);
        pllcfgr |= RCC_PLLCFGR_PLLREN;

        if (pll.p != 0u) {
            pllcfgr |= ((pll.p - 1u) << RCC_PLLCFGR_PLLP_Pos)
                     | RCC_PLLCFGR_PLLPEN;
        }
        if (pll.q != 0u) {
            pllcfgr |= ((pll.q - 1u) << RCC_PLLCFGR_PLLQ_Pos)
                     | RCC_PLLCFGR_PLLQEN;
        }

        RCC->PLLCFGR = pllcfgr;
        RCC->CR |= RCC_CR_PLLON;
        while ((RCC->CR & RCC_CR_PLLRDY) == 0u) {
        }
    }

    static constexpr std::uint32_t pll_source_code(PllInput input) {
        switch (input) {
        case PllInput::Msi: return 1u;
        case PllInput::Hsi16: return 2u;
        case PllInput::Hse32: return 3u;
        }
        return 0u;
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
};

} // namespace stm32::hw
