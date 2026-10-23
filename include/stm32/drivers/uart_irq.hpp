#pragma once

#include <cstddef>

namespace stm32::drivers {

// Fixed-size dispatcher for one UART interrupt vector.
//
// Some families share an IRQ between UART instances. Each registered driver
// checks its own status registers and returns false when the interrupt is not
// its own. No heap, no virtual functions.
template <auto Irq>
class UartIrqRouter {
public:
    using Handler = bool (*)();

    static void attach(Handler handler) {
        for (std::size_t i = 0; i < kCapacity; ++i) {
            if (handlers_[i] == handler) {
                return;
            }
        }
        for (std::size_t i = 0; i < kCapacity; ++i) {
            if (handlers_[i] == nullptr) {
                handlers_[i] = handler;
                return;
            }
        }
    }

    static void dispatch() {
        for (std::size_t i = 0; i < kCapacity; ++i) {
            if (handlers_[i] != nullptr) {
                (void)handlers_[i]();
            }
        }
    }

private:
    static constexpr std::size_t kCapacity = 4;
    static inline Handler handlers_[kCapacity] = {};
};

} // namespace stm32::drivers
