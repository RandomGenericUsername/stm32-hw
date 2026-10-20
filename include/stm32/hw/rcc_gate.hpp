#pragma once

#include <cstddef>
#include <cstdint>

namespace stm32::hw {

// Reusable RCC gate for families whose enable/reset registers are simple
// 32-bit bitfields (WL55, F4, G0, L4, H7, ...).
//
// Offsets and bit positions are supplied by the family clock implementation.
// They normally come from CMSIS:
//   offsetof(RCC_TypeDef, AHB2ENR)
//   RCC_AHB2ENR_GPIOBEN_Pos
template <
    std::uintptr_t RccBase,
    std::size_t EnableOffset,
    std::uint8_t EnableBit,
    std::size_t ResetOffset,
    std::uint8_t ResetBit>
class RccGate {
    static_assert(EnableBit < 32, "enable bit must be in 0..31");
    static_assert(ResetBit < 32, "reset bit must be in 0..31");

public:
    RccGate() = delete;
    RccGate(const RccGate&) = delete;
    RccGate& operator=(const RccGate&) = delete;

    static void enable() {
        *reg<EnableOffset>() |= (1u << EnableBit);
    }

    static void disable() {
        *reg<EnableOffset>() &= ~(1u << EnableBit);
    }

    static bool is_enabled() {
        return (*reg<EnableOffset>() & (1u << EnableBit)) != 0;
    }

    static void reset() {
        auto* r = reg<ResetOffset>();
        *r |= (1u << ResetBit);
        *r &= ~(1u << ResetBit);
    }

private:
    template <std::size_t Offset>
    static volatile std::uint32_t* reg() {
        return reinterpret_cast<volatile std::uint32_t*>(RccBase + Offset);
    }
};

} // namespace stm32::hw
