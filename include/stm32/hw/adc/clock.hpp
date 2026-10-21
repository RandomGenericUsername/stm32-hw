#pragma once

#include <cstdint>

#include "stm32/hw/clock/config.hpp"

namespace stm32::hw {

template <class Traits>
struct AdcClockConfig {
    using AdcSource = typename Traits::AdcSource;
    using AdcClockMode = typename Traits::AdcClockMode;
    using AdcPrescaler = typename Traits::AdcPrescaler;

    AdcSource source;
    AdcClockMode mode;
    AdcPrescaler prescaler;

    static constexpr std::uint32_t prescaler_divider(AdcPrescaler p) {
        switch (p) {
        case AdcPrescaler::Div1: return 1u;
        case AdcPrescaler::Div2: return 2u;
        case AdcPrescaler::Div4: return 4u;
        case AdcPrescaler::Div6: return 6u;
        case AdcPrescaler::Div8: return 8u;
        case AdcPrescaler::Div10: return 10u;
        case AdcPrescaler::Div12: return 12u;
        case AdcPrescaler::Div16: return 16u;
        case AdcPrescaler::Div32: return 32u;
        case AdcPrescaler::Div64: return 64u;
        case AdcPrescaler::Div128: return 128u;
        case AdcPrescaler::Div256: return 256u;
        }
        return 1u;
    }

    static constexpr std::uint32_t mode_divider(AdcClockMode m) {
        switch (m) {
        case AdcClockMode::Asynchronous: return 1u;
        case AdcClockMode::PclkDiv2: return 2u;
        case AdcClockMode::PclkDiv4: return 4u;
        case AdcClockMode::Pclk: return 1u;
        }
        return 1u;
    }

    constexpr std::uint32_t clock_hz(
        const BasicSystemClockConfig<Traits>& sys) const {
        if (mode == AdcClockMode::Asynchronous) {
            return Traits::adc_source_hz(sys.source, source)
                 / prescaler_divider(prescaler);
        }
        return sys.pclk2_hz() / mode_divider(mode);
    }

    constexpr bool valid(
        const BasicSystemClockConfig<Traits>& sys) const {
        return source != AdcSource::None
            && clock_hz(sys) <= Traits::max_adc_hz;
    }
};

} // namespace stm32::hw
