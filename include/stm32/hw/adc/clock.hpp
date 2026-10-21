#pragma once

#include <cstdint>

#include "stm32/hw/clock/config.hpp"

namespace stm32::hw {

// ADC clock policy. AdcTraits owns the ADC-specific choices (source, mode,
// prescaler, maximum ADC clock). The system clock config is used only for
// frequency queries and validation.
template <class AdcTraits>
struct AdcClockConfig {
    using Source = typename AdcTraits::Source;
    using Mode = typename AdcTraits::Mode;
    using Prescaler = typename AdcTraits::Prescaler;

    Source source;
    Mode mode;
    Prescaler prescaler;

    static constexpr std::uint32_t prescaler_divider(Prescaler p) {
        switch (p) {
        case Prescaler::Div1: return 1u;
        case Prescaler::Div2: return 2u;
        case Prescaler::Div4: return 4u;
        case Prescaler::Div6: return 6u;
        case Prescaler::Div8: return 8u;
        case Prescaler::Div10: return 10u;
        case Prescaler::Div12: return 12u;
        case Prescaler::Div16: return 16u;
        case Prescaler::Div32: return 32u;
        case Prescaler::Div64: return 64u;
        case Prescaler::Div128: return 128u;
        case Prescaler::Div256: return 256u;
        }
        return 1u;
    }

    static constexpr std::uint32_t mode_divider(Mode m) {
        switch (m) {
        case Mode::Asynchronous: return 1u;
        case Mode::PclkDiv2: return 2u;
        case Mode::PclkDiv4: return 4u;
        case Mode::Pclk: return 1u;
        }
        return 1u;
    }

    template <class ClockTraits>
    constexpr std::uint32_t clock_hz(
        const BasicSystemClockConfig<ClockTraits>& sys) const {
        if (mode == Mode::Asynchronous) {
            switch (source) {
            case Source::None:
                return 0u;
            case Source::Hsi16:
                return ClockTraits::hsi16_hz / prescaler_divider(prescaler);
            case Source::PllP:
                return sys.pll_p_hz() / prescaler_divider(prescaler);
            case Source::Sysclk:
                return sys.sysclk_hz() / prescaler_divider(prescaler);
            }
            return 0u;
        }
        return sys.pclk2_hz() / mode_divider(mode);
    }

    template <class ClockTraits>
    constexpr bool valid(
        const BasicSystemClockConfig<ClockTraits>& sys) const {
        const std::uint32_t hz = clock_hz(sys);
        return source != Source::None
            && hz != 0u
            && hz <= AdcTraits::max_clock_hz;
    }
};

} // namespace stm32::hw
