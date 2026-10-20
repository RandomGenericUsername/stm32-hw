#pragma once

#include <cstdint>

#include <stm32wl55xx.h>

#include "stm32/hw/gpio/config.hpp"

namespace stm32::hw {

// MODER-based GPIO layout shared by WL55, F4, G0, H7, L4, and other modern
// STM32 families.
//
// F1/F2-class parts use CRL/CRH instead; they get a separate LegacyLayout
// with the same Port/Pin API.
class ModernLayout {
public:
    ModernLayout() = delete;
    ModernLayout(const ModernLayout&) = delete;
    ModernLayout& operator=(const ModernLayout&) = delete;

    static void configure(GPIO_TypeDef* port,
                          std::uint8_t pin,
                          const GpioConfig& cfg) {
        const std::uint32_t s2 = 2u * pin;

        port->MODER = (port->MODER & ~(3u << s2))
                    | (mode_code(cfg.mode) << s2);

        port->OTYPER = (port->OTYPER & ~(1u << pin))
                     | (output_code(cfg.output) << pin);

        port->OSPEEDR = (port->OSPEEDR & ~(3u << s2))
                      | (speed_code(cfg.speed) << s2);

        port->PUPDR = (port->PUPDR & ~(3u << s2))
                    | (pull_code(cfg.pull) << s2);

        if (cfg.mode == Mode::Alternate) {
            const std::uint32_t index = pin >> 3;
            const std::uint32_t shift = 4u * (pin & 7u);

            port->AFR[index] = (port->AFR[index] & ~(0xFu << shift))
                             | ((static_cast<std::uint32_t>(
                                     cfg.alternate_function) & 0xFu)
                                << shift);
        }
    }

private:
    static constexpr std::uint32_t mode_code(Mode m) {
        return static_cast<std::uint32_t>(m);
    }

    static constexpr std::uint32_t output_code(OutputType t) {
        return static_cast<std::uint32_t>(t);
    }

    static constexpr std::uint32_t speed_code(Speed s) {
        return static_cast<std::uint32_t>(s);
    }

    static constexpr std::uint32_t pull_code(Pull p) {
        return static_cast<std::uint32_t>(p);
    }
};

} // namespace stm32::hw
