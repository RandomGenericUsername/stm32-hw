#pragma once

#include <cstdint>

#include "stm32/device/wl55/clock.hpp"
#include "stm32/hw/adc/clock.hpp"
#include "stm32/hw/clock/config.hpp"

namespace stm32::board::nucleo_wl55jc {

using hw::AdcClockConfig;
using hw::AhbDivider;
using hw::ApbDivider;
using hw::BasicSystemClockConfig;

// NUCLEO-WL55JC (MB1389) board clock facts.
// HSE default: X4 32 MHz TCXO (UM2592 §6.5.1).
// LSE default: X2 32.768 kHz crystal (UM2592 §6.5.2).
struct Clock : device::wl55::ClockTraits {
    static constexpr std::uint32_t hse_hz = 32000000u;
    static constexpr bool hse_tcxo = true; // X4 TCXO default
    static constexpr std::uint32_t lse_hz = 32768u;

    static constexpr std::uint32_t source_hz(const Source& source) {
        return device::wl55::ClockTraits::source_hz(source, hse_hz);
    }

    static constexpr bool valid_source(const Source& source) {
        return device::wl55::ClockTraits::valid_source(source, hse_hz);
    }

    static constexpr std::uint32_t adc_source_hz(
        const Source& sys_source, AdcSource adc_source) {
        return device::wl55::ClockTraits::adc_source_hz(
            sys_source, adc_source, hse_hz);
    }
};

inline constexpr BasicSystemClockConfig<Clock> msi_4mhz = {
    Clock::Source::msi(Clock::MsiRange::R4M),
    AhbDivider::Div1,
    ApbDivider::Div1,
    ApbDivider::Div1,
};

inline constexpr BasicSystemClockConfig<Clock> hsi16_16mhz = {
    Clock::Source::hsi16(),
    AhbDivider::Div1,
    ApbDivider::Div1,
    ApbDivider::Div1,
};

inline constexpr BasicSystemClockConfig<Clock> hse32_32mhz = {
    Clock::Source::hse32(),
    AhbDivider::Div1,
    ApbDivider::Div1,
    ApbDivider::Div1,
};

// HSI16 -> PLL: 16 MHz / 1 * 12 = 192 MHz VCO.
// PLLR = 4 -> 48 MHz SYSCLK; PLLP = 6 -> 32 MHz ADC source.
inline constexpr BasicSystemClockConfig<Clock> pll_48mhz = {
    Clock::Source::pll(Clock::PllConfig{
        Clock::PllConfig::Input::Hsi16,
        Clock::MsiRange::R4M,
        1u,
        12u,
        4u,
        6u,
        4u,
    }),
    AhbDivider::Div1,
    ApbDivider::Div1,
    ApbDivider::Div1,
};

static_assert(msi_4mhz.valid());
static_assert(hsi16_16mhz.valid());
static_assert(hse32_32mhz.valid());
static_assert(pll_48mhz.valid());

inline constexpr AdcClockConfig<Clock> adc_hsi16_16mhz = {
    Clock::AdcSource::Hsi16,
    Clock::AdcClockMode::Asynchronous,
    Clock::AdcPrescaler::Div1,
};

inline constexpr AdcClockConfig<Clock> adc_pllp_32mhz = {
    Clock::AdcSource::PllP,
    Clock::AdcClockMode::Asynchronous,
    Clock::AdcPrescaler::Div1,
};

inline constexpr AdcClockConfig<Clock> adc_sysclk_24mhz = {
    Clock::AdcSource::Sysclk,
    Clock::AdcClockMode::Asynchronous,
    Clock::AdcPrescaler::Div2,
};

static_assert(adc_hsi16_16mhz.valid(hsi16_16mhz));
static_assert(adc_pllp_32mhz.valid(pll_48mhz));
static_assert(adc_sysclk_24mhz.valid(pll_48mhz));

} // namespace stm32::board::nucleo_wl55jc
