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

using LedHw = stm32::device::wl55::Led;
using Led = stm32::drivers::Led<LedHw>;

void app_main() {
    Led led;
    led.init();

    for (;;) {
        led.toggle();
        // delay
    }
}
```

`LedHw` is the stateless hardware facade. `Led` is the stateful driver.

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
- Interrupts: `hw::Exti` / `hw::Nvic`, WL55 `WlExti` / `WlNvic`,
  `device::wl55::ButtonSw1` (PA0, EXTI0, falling edge)
- Compile check: `tests/compile_check.cpp`
- Design notes: `docs/design/interrupts.md`
