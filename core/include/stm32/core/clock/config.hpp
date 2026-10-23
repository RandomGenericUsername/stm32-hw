#pragma once

#include <cstdint>

namespace stm32::hw {

// RCC_CFGR.HPRE encodings (WL55/F4/L4-style AHB prescaler).
enum class AhbDivider : std::uint8_t {
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
enum class ApbDivider : std::uint8_t {
    Div1 = 0,
    Div2 = 4,
    Div4 = 5,
    Div8 = 6,
    Div16 = 7,
};

constexpr std::uint32_t divider(AhbDivider d) {
    switch (d) {
    case AhbDivider::Div1: return 1u;
    case AhbDivider::Div2: return 2u;
    case AhbDivider::Div4: return 4u;
    case AhbDivider::Div8: return 8u;
    case AhbDivider::Div16: return 16u;
    case AhbDivider::Div64: return 64u;
    case AhbDivider::Div128: return 128u;
    case AhbDivider::Div256: return 256u;
    case AhbDivider::Div512: return 512u;
    }
    return 1u;
}

constexpr std::uint32_t divider(ApbDivider d) {
    switch (d) {
    case ApbDivider::Div1: return 1u;
    case ApbDivider::Div2: return 2u;
    case ApbDivider::Div4: return 4u;
    case ApbDivider::Div8: return 8u;
    case ApbDivider::Div16: return 16u;
    }
    return 1u;
}

// Declarative clock tree for families with one AHB and two APB domains
// (WL55, F4, L4). The traits type supplies the sources, frequency table,
// limits, PLL validation and flash-latency rules.
template <class Traits>
struct BasicSystemClockConfig {
    typename Traits::Source source;
    AhbDivider ahb;
    ApbDivider apb1;
    ApbDivider apb2;

    constexpr std::uint32_t sysclk_hz() const {
        return Traits::source_hz(source);
    }

    constexpr std::uint32_t hclk_hz() const {
        return sysclk_hz() / divider(ahb);
    }

    constexpr std::uint32_t pclk1_hz() const {
        return hclk_hz() / divider(apb1);
    }

    constexpr std::uint32_t pclk2_hz() const {
        return hclk_hz() / divider(apb2);
    }

    constexpr std::uint32_t flash_wait_states() const {
        return Traits::flash_latency(hclk_hz());
    }

    // PLLP is only meaningful when the system source is the PLL; otherwise
    // this is zero. The traits type supplies the device/board frequency math.
    constexpr std::uint32_t pll_p_hz() const {
        return Traits::pll_p_hz(source.pll_config);
    }

    constexpr bool valid() const {
        return Traits::valid_source(source)
            && sysclk_hz() <= Traits::max_sysclk_hz
            && hclk_hz() <= Traits::max_hclk_hz
            && pclk1_hz() <= Traits::max_pclk1_hz
            && pclk2_hz() <= Traits::max_pclk2_hz
            && flash_wait_states() <= Traits::max_flash_latency;
    }
};

} // namespace stm32::hw
