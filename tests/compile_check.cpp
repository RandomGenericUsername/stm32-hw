// Compile-time check for the WL55 clock + GPIO + EXTI/NVIC building blocks.
//
// This translation unit is meant to be compiled with the consuming project's
// CMSIS include paths. It does not need to link.

#include "stm32/device/wl55/clock.hpp"
#include "stm32/device/wl55/gpio.hpp"
#include "stm32/drivers/led.hpp"
#include "stm32/hw/clock/wl55.hpp"
#include "stm32/hw/exti/wl55.hpp"
#include "stm32/hw/gpio/pin.hpp"
#include "stm32/hw/nvic/wl55.hpp"

using WlClock = stm32::hw::WlClock<stm32::device::wl55::ClockTraits>;

// Board wiring intentionally lives here, in the consumer — not in the library.
using LedHw = stm32::hw::Pin<stm32::device::wl55::GpioB, 15, WlClock>;
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

#if defined(__ARM_ARCH_7EM__)
    // App-side EXTI composition: primitives only; no board policy in the lib.
    stm32::hw::WlExti::configure<stm32::device::wl55::GpioA, 0>(
        stm32::hw::Edge::Falling);
    stm32::hw::WlExti::enable<0>();
    (void)stm32::hw::WlExti::is_pending<0>();
    stm32::hw::WlExti::disable<0>();
    stm32::hw::WlExti::clear_pending<0>();

    stm32::hw::WlNvic::set_priority(EXTI0_IRQn, 15);
    stm32::hw::WlNvic::enable(EXTI0_IRQn);
    stm32::hw::WlNvic::disable(EXTI0_IRQn);
#endif
}
