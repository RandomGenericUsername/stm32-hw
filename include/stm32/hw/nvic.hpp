#pragma once

#include <cstdint>

namespace stm32::hw {

// Abstract NVIC facade.
//
// Type-level singleton; the concrete implementation supplies the static
// methods and is responsible for including the CMSIS core header.
template <class Impl>
class Nvic {
public:
    Nvic() = delete;
    Nvic(const Nvic&) = delete;
    Nvic& operator=(const Nvic&) = delete;

    template <class Irq>
    static void enable(Irq irq) {
        Impl::enable(irq);
    }

    template <class Irq>
    static void disable(Irq irq) {
        Impl::disable(irq);
    }

    template <class Irq>
    static void clear_pending(Irq irq) {
        Impl::clear_pending(irq);
    }

    template <class Irq>
    static void set_priority(Irq irq, std::uint32_t priority) {
        Impl::set_priority(irq, priority);
    }
};

} // namespace stm32::hw
