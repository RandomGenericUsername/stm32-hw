#pragma once

#include <cstdint>

#include <stm32f4xx.h>

#include "stm32/hw/nvic.hpp"

namespace stm32::hw {

class F4Nvic final : public Nvic<F4Nvic> {
public:
    F4Nvic() = delete;
    F4Nvic(const F4Nvic&) = delete;
    F4Nvic& operator=(const F4Nvic&) = delete;

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
