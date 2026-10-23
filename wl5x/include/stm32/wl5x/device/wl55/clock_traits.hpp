#pragma once

#include <cstdint>

namespace stm32::device::wl55 {

// STM32WL55 clock facts (RM0453 §7).
struct ClockTraits {
    enum class MsiRange : std::uint8_t {
        R100k,
        R200k,
        R400k,
        R800k,
        R1M,
        R2M,
        R4M,
        R8M,
        R16M,
        R24M,
        R32M,
        R48M,
    };

    struct PllConfig {
        enum class Input : std::uint8_t {
            Msi,
            Hsi16,
            Hse32,
        };

        Input input;
        MsiRange msi;
        std::uint8_t m; // 1..8
        std::uint8_t n; // 6..127
        std::uint8_t r; // 2..8
        std::uint8_t p; // 2..32, 0 = disabled
        std::uint8_t q; // 2..8,  0 = disabled
    };

    struct Source {
        enum class Kind : std::uint8_t {
            Msi,
            Hsi16,
            Hse32,
            Pll,
        };

        Kind kind;
        MsiRange msi_range;
        bool hse_div2;
        PllConfig pll_config;

        static constexpr Source msi(MsiRange range) {
            return Source{Kind::Msi, range, false, {}};
        }

        static constexpr Source hsi16() {
            return Source{Kind::Hsi16, MsiRange::R4M, false, {}};
        }

        static constexpr Source hse32(bool divide_by_2 = false) {
            return Source{Kind::Hse32, MsiRange::R4M, divide_by_2, {}};
        }

        static constexpr Source pll(PllConfig config) {
            return Source{Kind::Pll, config.msi, false, config};
        }
    };

    static constexpr std::uint32_t hsi16_hz = 16000000u;
    static constexpr bool hse_tcxo = false;
    static constexpr std::uint32_t max_sysclk_hz = 48000000u;
    static constexpr std::uint32_t max_hclk_hz = 48000000u;
    static constexpr std::uint32_t max_pclk1_hz = 48000000u;
    static constexpr std::uint32_t max_pclk2_hz = 48000000u;
    static constexpr std::uint32_t max_flash_latency = 2u;

    static constexpr std::uint32_t msi_hz(MsiRange range) {
        switch (range) {
        case MsiRange::R100k: return 100000u;
        case MsiRange::R200k: return 200000u;
        case MsiRange::R400k: return 400000u;
        case MsiRange::R800k: return 800000u;
        case MsiRange::R1M: return 1000000u;
        case MsiRange::R2M: return 2000000u;
        case MsiRange::R4M: return 4000000u;
        case MsiRange::R8M: return 8000000u;
        case MsiRange::R16M: return 16000000u;
        case MsiRange::R24M: return 24000000u;
        case MsiRange::R32M: return 32000000u;
        case MsiRange::R48M: return 48000000u;
        }
        return 0u;
    }

    static constexpr std::uint32_t msi_code(MsiRange range) {
        switch (range) {
        case MsiRange::R100k: return 0u;
        case MsiRange::R200k: return 1u;
        case MsiRange::R400k: return 2u;
        case MsiRange::R800k: return 3u;
        case MsiRange::R1M: return 4u;
        case MsiRange::R2M: return 5u;
        case MsiRange::R4M: return 6u;
        case MsiRange::R8M: return 7u;
        case MsiRange::R16M: return 8u;
        case MsiRange::R24M: return 9u;
        case MsiRange::R32M: return 10u;
        case MsiRange::R48M: return 11u;
        }
        return 0u;
    }

    static constexpr std::uint32_t pll_input_hz(const PllConfig& pll,
                                                std::uint32_t hse_hz) {
        switch (pll.input) {
        case PllConfig::Input::Msi:
            return msi_hz(pll.msi) / pll.m;
        case PllConfig::Input::Hsi16:
            return hsi16_hz / pll.m;
        case PllConfig::Input::Hse32:
            return hse_hz / pll.m;
        }
        return 0u;
    }

    static constexpr std::uint32_t pll_vco_hz(const PllConfig& pll,
                                              std::uint32_t hse_hz) {
        return pll_input_hz(pll, hse_hz) * pll.n;
    }

    static constexpr std::uint32_t pll_r_hz(const PllConfig& pll,
                                            std::uint32_t hse_hz) {
        return pll_vco_hz(pll, hse_hz) / pll.r;
    }

    static constexpr std::uint32_t pll_p_hz(const PllConfig& pll,
                                            std::uint32_t hse_hz) {
        return pll.p == 0u ? 0u : pll_vco_hz(pll, hse_hz) / pll.p;
    }

    static constexpr std::uint32_t pll_q_hz(const PllConfig& pll,
                                            std::uint32_t hse_hz) {
        return pll.q == 0u ? 0u : pll_vco_hz(pll, hse_hz) / pll.q;
    }

    static constexpr std::uint32_t source_hz(const Source& source,
                                             std::uint32_t hse_hz) {
        switch (source.kind) {
        case Source::Kind::Msi:
            return msi_hz(source.msi_range);
        case Source::Kind::Hsi16:
            return hsi16_hz;
        case Source::Kind::Hse32:
            return hse_hz / (source.hse_div2 ? 2u : 1u);
        case Source::Kind::Pll:
            return pll_r_hz(source.pll_config, hse_hz);
        }
        return 0u;
    }

    static constexpr bool valid_source(const Source& source,
                                       std::uint32_t hse_hz) {
        if (source.kind != Source::Kind::Pll) {
            return true;
        }

        const PllConfig& pll = source.pll_config;
        if (pll.m < 1u || pll.m > 8u || pll.n < 6u || pll.n > 127u
            || pll.r < 2u || pll.r > 8u) {
            return false;
        }
        if ((pll.p != 0u && (pll.p < 2u || pll.p > 32u))
            || (pll.q != 0u && (pll.q < 2u || pll.q > 8u))) {
            return false;
        }

        const std::uint32_t input = pll_input_hz(pll, hse_hz);
        const std::uint32_t vco = pll_vco_hz(pll, hse_hz);
        if (input < 2660000u || input > 16000000u) {
            return false;
        }
        if (vco < 96000000u || vco > 344000000u) {
            return false;
        }
        if (pll_r_hz(pll, hse_hz) > max_sysclk_hz) {
            return false;
        }
        if (pll_p_hz(pll, hse_hz) > max_sysclk_hz
            || pll_q_hz(pll, hse_hz) > max_sysclk_hz) {
            return false;
        }
        return true;
    }

    static constexpr std::uint32_t flash_latency(std::uint32_t hclk_hz) {
        if (hclk_hz <= 18000000u) {
            return 0u;
        }
        if (hclk_hz <= 36000000u) {
            return 1u;
        }
        return 2u;
    }
};

} // namespace stm32::device::wl55
