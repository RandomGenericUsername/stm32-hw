#pragma once

#include <stm32wl55xx.h>

#include "stm32/hw/clock/wl55.hpp"
#include "stm32/hw/gpio/pin.hpp"

namespace stm32::device::wl55 {

// Device descriptors: identity plus the CMSIS register block pointer.
// No clock gate, bus name, or register offset lives here.

class GpioA final {
public:
    static constexpr hw::Peripheral id = hw::Peripheral::GpioA;
    static GPIO_TypeDef* regs() { return GPIOA; }

private:
    GpioA() = delete;
};

class GpioB final {
public:
    static constexpr hw::Peripheral id = hw::Peripheral::GpioB;
    static GPIO_TypeDef* regs() { return GPIOB; }

private:
    GpioB() = delete;
};

class GpioC final {
public:
    static constexpr hw::Peripheral id = hw::Peripheral::GpioC;
    static GPIO_TypeDef* regs() { return GPIOC; }

private:
    GpioC() = delete;
};

class GpioH final {
public:
    static constexpr hw::Peripheral id = hw::Peripheral::GpioH;
    static GPIO_TypeDef* regs() { return GPIOH; }

private:
    GpioH() = delete;
};

// Nucleo-WL55JC user LED LD1 (blue) is PB15, active high.
using Led = hw::Pin<GpioB, 15, hw::WlClock>;

} // namespace stm32::device::wl55
