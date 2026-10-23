#pragma once

#include <cstdint>

#include "stm32/wl5x/device/wl55/adc_clock.hpp"
#include "stm32/wl5x/device/wl55/clock_traits.hpp"
#include "stm32/core/clock/adc_clock.hpp"
#include "stm32/core/clock/config.hpp"

namespace stm32::board::nucleo_wl55jc {

using AdcClock = device::wl55::AdcClockTraits;
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

    static constexpr std::uint32_t pll_p_hz(const PllConfig& pll) {
        return device::wl55::ClockTraits::pll_p_hz(pll, hse_hz);
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

inline constexpr AdcClockConfig<AdcClock> adc_hsi16_16mhz = {
    AdcClock::Source::Hsi16,
    AdcClock::Mode::Asynchronous,
    AdcClock::Prescaler::Div1,
};

inline constexpr AdcClockConfig<AdcClock> adc_pllp_32mhz = {
    AdcClock::Source::PllP,
    AdcClock::Mode::Asynchronous,
    AdcClock::Prescaler::Div1,
};

inline constexpr AdcClockConfig<AdcClock> adc_sysclk_24mhz = {
    AdcClock::Source::Sysclk,
    AdcClock::Mode::Asynchronous,
    AdcClock::Prescaler::Div2,
};

static_assert(adc_hsi16_16mhz.valid(hsi16_16mhz));
static_assert(adc_pllp_32mhz.valid(pll_48mhz));
static_assert(adc_sysclk_24mhz.valid(pll_48mhz));

} // namespace stm32::board::nucleo_wl55jc
