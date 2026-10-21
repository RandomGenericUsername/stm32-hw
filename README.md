# stm32-hw

Cross-family bare-metal C++ hardware abstraction library for STM32 MCUs.

The library is split into three layers:

```text
drivers      stateful classes (Led, Uart, Spi, ...)
hw           stateless type-level hardware facade (WlClock, GpioB, Pin)
device       family/device descriptors (CMSIS pointers + peripheral identities)
```

The `hw` and `device` layers are header-only. Drivers may become compiled
sources later; the first slice is header-only.

## Design rules

- `hw::` types are **type-level singletons**: static methods, deleted
  constructors, no state. The hardware peripheral is the singleton.
- `drivers::` types are **stateful objects**: configuration, state machines,
  buffers, callbacks, ownership.
- Clock/RCC logic lives only in the clock implementation (`hw::WlClock`).
- GPIO register layout lives only in the port/layout types
  (`hw::Port`, `hw::ModernLayout`).
- A `Pin` composes a port and a clock; it owns neither.
- `device::` contains **MCU facts only** (register pointers, peripheral IDs).
  Board wiring and application flows — which pin is a button, pull/edge/IRQ
  priority, ISR behavior — belong to the consuming application.
- No register addresses, offsets, or bit numbers are hardcoded in the generic
  layers. They come from CMSIS structs, `offsetof`, and CMSIS bit-position
  macros in the family/device layer.

## CMSIS dependency

This library does not vendor CMSIS. The consuming project supplies the CMSIS
Core and device headers on its include path:

```make
CMSIS_INC = -Ivendor/CMSIS/CMSIS/Core/Include \
            -Ivendor/cmsis-device-stm32wl55jc/Include
```

The WL55 headers are included as `<stm32wl55xx.h>`.

## Usage

```cpp
#include <stm32/device/wl55/gpio.hpp>
#include <stm32/drivers/led.hpp>

using LedPin = stm32::hw::Pin<stm32::device::wl55::GpioB, 15, stm32::hw::WlClock>;
using Led = stm32::drivers::Led<LedPin>;

void app_main() {
    Led led;
    led.init();

    for (;;) {
        led.toggle();
        // delay
    }
}
```

`LedPin` is the stateless hardware facade, defined by the application. `Led` is
the generic stateful driver. The library has no board-specific LED alias.

### Interrupts

The application composes the flow from primitives:

```cpp
#include <stm32/hw/exti/wl55.hpp>
#include <stm32/hw/nvic/wl55.hpp>

using ButtonPin = stm32::hw::Pin<stm32::device::wl55::GpioA, 0, stm32::hw::WlClock>;

void button_init() {
    ButtonPin::init(stm32::hw::GpioConfig{
        stm32::hw::Mode::Input,
        stm32::hw::OutputType::PushPull,
        stm32::hw::Speed::Low,
        stm32::hw::Pull::Up,
        0,
    });

    stm32::hw::WlExti::configure<stm32::device::wl55::GpioA, 0>(
        stm32::hw::Edge::Falling);
    stm32::hw::WlExti::clear_pending<0>();
    stm32::hw::WlExti::enable<0>();

    stm32::hw::WlNvic::clear_pending(EXTI0_IRQn);
    stm32::hw::WlNvic::set_priority(EXTI0_IRQn, 15);
    stm32::hw::WlNvic::enable(EXTI0_IRQn);
}

extern "C" void EXTI0_IRQHandler(void) {
    stm32::hw::WlExti::clear_pending<0>();
    // signal the main loop (STA pattern)
}
```

Pin choice, pull, edge, priority, ISR symbol, and ISR behaviour stay in the
application.

## Build integration

The library is header-only for the first slice. Add the include directory and
compile/link C++ with `arm-none-eabi-g++`:

```make
STM32_HW_DIR = third_party/stm32-hw
CXXFLAGS += -I$(STM32_HW_DIR)/include -std=c++20 -fno-exceptions -fno-rtti
```

No linker archive is required while it remains header-only. If compiled
driver sources are added later, build per-core objects or per-core archives
because the WL55 M4 and M0+ use different `-mcpu` flags.

## Status

First vertical slice:

- WL55 clock gate: `RCC_TypeDef` + `offsetof` + CMSIS bit macros
- GPIO: `ModernLayout`, `Port`, `Pin`
- Device: `GpioA`, `GpioB`, `GpioC`, `GpioH`
- Driver: stateful `Led`
- Interrupts: `hw::Exti` / `hw::Nvic` primitives, WL55 `WlExti` / `WlNvic`
  (no board-specific button class)
- ADC: `hw::Adc`, `F4Adc` / `H7Adc` / `WlAdc`; device descriptors
  `device::f4::Adc1`, `device::h7::Adc1/2/3`, `device::wl55::Adc1`;
  12-bit single conversion, software trigger, polling
- Compile check: `tests/compile_check.cpp`
- ADC compile checks: `tests/adc_compile_f4.cpp`, `adc_compile_h7.cpp`,
  `adc_compile_wl55.cpp`
- Design notes: `docs/design/interrupts.md`, `docs/design/adc.md`
