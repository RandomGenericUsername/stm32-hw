#pragma once

#include <cstddef>
#include <cstdint>

#include <stm32wl55xx.h>

#include "stm32/hw/exti.hpp"
#include "stm32/hw/peripheral.hpp"

namespace stm32::hw {

// WL55 external-interrupt implementation (CPU1 / Cortex-M4 view).
//
// WL55 specifics encoded here:
//   * GPIO line -> port routing lives in SYSCFG->EXTICR[0..3], not in EXTI.
//   * Only GPIOA, GPIOB, and GPIOC can be routed to EXTI lines 0..15.
//   * Trigger selection is EXTI->RTSR1 / EXTI->FTSR1.
//   * CPU1 interrupt mask is EXTI->IMR1; pending is EXTI->PR1.
class WlExti final : public Exti<WlExti> {
public:
    WlExti() = delete;
    WlExti(const WlExti&) = delete;
    WlExti& operator=(const WlExti&) = delete;

    template <class PortDesc, std::uint8_t Line>
    static void configure(Edge edge) {
        static_assert(Line < 16, "WL55 GPIO EXTI line must be in 0..15");

        constexpr std::uint32_t port = port_code<PortDesc>();
        constexpr std::uint32_t stride = SYSCFG_EXTICR1_EXTI1_Pos; // 4 bits/line
        constexpr std::uint32_t field_mask = SYSCFG_EXTICR1_EXTI0_Msk;
        constexpr std::uint32_t index = Line / 4u;                 // 4 lines/EXTICR
        constexpr std::uint32_t shift = (Line % 4u) * stride;

        SYSCFG->EXTICR[index] =
            (SYSCFG->EXTICR[index] & ~(field_mask << shift))
            | (port << shift);

        constexpr std::uint32_t mask = 1u << Line;
        const std::uint8_t flags = static_cast<std::uint8_t>(edge);

        if ((flags & static_cast<std::uint8_t>(Edge::Rising)) != 0) {
            EXTI->RTSR1 |= mask;
        } else {
            EXTI->RTSR1 &= ~mask;
        }

        if ((flags & static_cast<std::uint8_t>(Edge::Falling)) != 0) {
            EXTI->FTSR1 |= mask;
        } else {
            EXTI->FTSR1 &= ~mask;
        }
    }

    template <std::uint8_t Line>
    static void enable() {
        static_assert(Line < 32, "WL55 EXTI line must be in 0..31");
        EXTI->IMR1 |= (1u << Line);
    }

    template <std::uint8_t Line>
    static void disable() {
        static_assert(Line < 32, "WL55 EXTI line must be in 0..31");
        EXTI->IMR1 &= ~(1u << Line);
    }

    template <std::uint8_t Line>
    static void clear_pending() {
        static_assert(Line < 32, "WL55 EXTI line must be in 0..31");
        EXTI->PR1 = (1u << Line); // write 1 to clear
    }

    template <std::uint8_t Line>
    static bool is_pending() {
        static_assert(Line < 32, "WL55 EXTI line must be in 0..31");
        return (EXTI->PR1 & (1u << Line)) != 0;
    }

private:
    template <class PortDesc>
    static constexpr std::uint32_t port_code() {
        if constexpr (PortDesc::id == Peripheral::GpioA) {
            return SYSCFG_EXTICR1_EXTI0_PA;
        } else if constexpr (PortDesc::id == Peripheral::GpioB) {
            return SYSCFG_EXTICR1_EXTI0_PB;
        } else if constexpr (PortDesc::id == Peripheral::GpioC) {
            return SYSCFG_EXTICR1_EXTI0_PC;
        } else {
            static_assert(PortDesc::id == Peripheral::GpioA,
                          "WL55 EXTI port must be GPIOA, GPIOB, or GPIOC");
        }
    }
};

} // namespace stm32::hw
