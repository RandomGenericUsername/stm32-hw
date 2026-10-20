#pragma once

#include <cstdint>

#include "stm32/hw/gpio/modern_layout.hpp"

namespace stm32::hw {

// Register-level GPIO port facade.
//
// PortDesc supplies:
//   static GPIO_TypeDef* regs();
//
// Layout supplies:
//   static void configure(GPIO_TypeDef*, std::uint8_t, const GpioConfig&);
template <class PortDesc, class Layout = ModernLayout>
class Port {
public:
    Port() = delete;
    Port(const Port&) = delete;
    Port& operator=(const Port&) = delete;

    static void configure(std::uint8_t pin, const GpioConfig& cfg) {
        Layout::configure(PortDesc::regs(), pin, cfg);
    }

    static void set(std::uint32_t mask) {
        PortDesc::regs()->BSRR = mask;
    }

    static void reset(std::uint32_t mask) {
        PortDesc::regs()->BSRR = mask << 16;
    }

    static void toggle(std::uint32_t mask) {
        const std::uint32_t odr = PortDesc::regs()->ODR;
        PortDesc::regs()->BSRR = ((odr & mask) << 16) | (~odr & mask);
    }

    static std::uint32_t read() {
        return PortDesc::regs()->IDR;
    }
};

} // namespace stm32::hw
