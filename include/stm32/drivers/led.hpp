#pragma once

#include "stm32/hw/gpio/config.hpp"

namespace stm32::drivers {

// Stateful LED driver built on a stateless hardware pin.
//
// HwPin supplies:
//   static void init(const hw::GpioConfig&);
//   static void set();
//   static void reset();
//   static void toggle();
template <class HwPin>
class Led {
public:
    Led() = default;
    Led(const Led&) = delete;
    Led& operator=(const Led&) = delete;

    void init() {
        HwPin::init(hw::GpioConfig{
            hw::Mode::Output,
            hw::OutputType::PushPull,
            hw::Speed::Low,
            hw::Pull::None,
            0,
        });

        HwPin::reset();
        on_ = false;
    }

    void on() {
        HwPin::set();
        on_ = true;
    }

    void off() {
        HwPin::reset();
        on_ = false;
    }

    void toggle() {
        if (on_) {
            off();
        } else {
            on();
        }
    }

    bool is_on() const {
        return on_;
    }

private:
    bool on_ = false;
};

} // namespace stm32::drivers
