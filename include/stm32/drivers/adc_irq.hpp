#pragma once

#include <cstddef>

namespace stm32::drivers {

// Fixed-size dispatcher for one ADC interrupt vector.
//
// ADC IRQ lines are shared on some families (F4 ADC1/2/3; H7 ADC1/2), so the
// vector cannot identify the source by itself. Each registered driver checks
// its own ADC ISR and returns false when the interrupt is not its own.
template <auto Irq>
class AdcIrqRouter {
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
