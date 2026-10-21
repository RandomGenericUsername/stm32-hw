#pragma once

#include <cstdint>

#include <stm32wl55xx.h>

#include "stm32/device/wl55/gpio.hpp"
#include "stm32/hw/exti/wl55.hpp"
#include "stm32/hw/nvic/wl55.hpp"

namespace stm32::device::wl55 {

// Nucleo-WL55JC B1 / BUTTON_SW1.
//
// UM2592 section 6.6.2: B1 is connected to PA0 by default (SB16 ON, SB15 OFF),
// active low, with the internal pull-up enabled. ST's BSP uses
// GPIO_MODE_IT_FALLING, so a press is a falling edge on EXTI line 0.
class ButtonSw1 final {
public:
    ButtonSw1() = delete;
    ButtonSw1(const ButtonSw1&) = delete;
    ButtonSw1& operator=(const ButtonSw1&) = delete;

    static constexpr hw::Edge edge = hw::Edge::Falling;
    static constexpr std::uint32_t irq_priority = 15; // lowest, as ST BSP

    static void init() {
        hw::Pin<GpioA, 0, hw::WlClock>::init(hw::GpioConfig{
            hw::Mode::Input,
            hw::OutputType::PushPull,
            hw::Speed::Low,
            hw::Pull::Up,
            0,
        });

        hw::WlExti::configure<GpioA, 0>(edge);
        hw::WlExti::clear_pending<0>();
        hw::WlExti::enable<0>();

        hw::WlNvic::clear_pending(EXTI0_IRQn);
        hw::WlNvic::set_priority(EXTI0_IRQn, irq_priority);
        hw::WlNvic::enable(EXTI0_IRQn);
    }

    static void clear_pending() {
        hw::WlExti::clear_pending<0>();
    }

    static bool is_pressed() {
        // Active low: pressed pulls PA0 to GND.
        return !hw::Pin<GpioA, 0, hw::WlClock>::read();
    }
};

} // namespace stm32::device::wl55
