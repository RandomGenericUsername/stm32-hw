#pragma once

#include <cstdint>

namespace stm32::device::wl55 {

// STM32WL55 ADC clock facts (RM0453 §18.3.5, DS13293 fADC limit).
struct AdcClockTraits {
    // RCC_CCIPR.ADCSEL encodings. The mux itself is in RCC.
    enum class Source : std::uint32_t {
        None = 0,
        Hsi16 = 1,
        PllP = 2,
        Sysclk = 3,
    };

    // ADC_CFGR2.CKMODE encodings.
    enum class Mode : std::uint32_t {
        Asynchronous = 0,
        PclkDiv2 = 1,
        PclkDiv4 = 2,
        Pclk = 3,
    };

    // ADC_CCR.PRESC encodings (asynchronous mode only).
    enum class Prescaler : std::uint32_t {
        Div1 = 0,
        Div2 = 1,
        Div4 = 2,
        Div6 = 3,
        Div8 = 4,
        Div10 = 5,
        Div12 = 6,
        Div16 = 7,
        Div32 = 8,
        Div64 = 9,
        Div128 = 10,
        Div256 = 11,
    };

    static constexpr std::uint32_t max_clock_hz = 35000000u;
};

} // namespace stm32::device::wl55
