#pragma once

#include <cstdint>

#include <stm32wl55xx.h>

#include "stm32/hw/nvic.hpp"

namespace stm32::hw {

// WL55 NVIC implementation. IRQ numbers come from the CMSIS IRQn_Type enum;
// the CMSIS core header provides the NVIC_* helpers.
class WlNvic final : public Nvic<WlNvic> {
public:
    WlNvic() = delete;
    WlNvic(const WlNvic&) = delete;
    WlNvic& operator=(const WlNvic&) = delete;

    template <class Irq>
    static void enable(Irq irq) {
        NVIC_EnableIRQ(irq);
    }

    template <class Irq>
    static void disable(Irq irq) {
        NVIC_DisableIRQ(irq);
    }

    template <class Irq>
    static void clear_pending(Irq irq) {
        NVIC_ClearPendingIRQ(irq);
    }

    template <class Irq>
    static void set_priority(Irq irq, std::uint32_t priority) {
        NVIC_SetPriority(irq, priority);
    }
};

} // namespace stm32::hw
