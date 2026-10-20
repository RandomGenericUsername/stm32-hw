// Compile-time check for the WL55 clock + GPIO vertical slice.
//
// This translation unit is meant to be compiled with the consuming project's
// CMSIS include paths. It does not need to link.

#include "stm32/device/wl55/gpio.hpp"
#include "stm32/drivers/led.hpp"

using LedHw = stm32::device::wl55::Led;
using LedDriver = stm32::drivers::Led<LedHw>;

void compile_check() {
    LedHw::init(stm32::hw::GpioConfig{
        stm32::hw::Mode::Output,
        stm32::hw::OutputType::PushPull,
        stm32::hw::Speed::Low,
        stm32::hw::Pull::None,
        0,
    });

    LedHw::set();
    LedHw::reset();
    LedHw::toggle();
    (void)LedHw::read();

    LedDriver led;
    led.init();
    led.on();
    led.off();
    led.toggle();
    (void)led.is_on();
}
