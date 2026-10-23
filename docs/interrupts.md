# Interrupts — EXTI + NVIC primitives (WL55)

Status: implemented (primitives); board composition lives in the application
(`wl55jc/src/app.cpp`).
Date: 2026-10-20

## Scope and boundary

The library provides **mechanisms**, not board flows:

- `hw::Exti` / `WlExti`: route a GPIO line to EXTI, select edge(s), mask,
  pending clear.
- `hw::Nvic` / `WlNvic`: enable/disable an IRQ, set priority, clear pending.
- `device::wl55`: **MCU facts only** — `GpioA/B/C/H` register pointers and
  peripheral IDs.

Board wiring and interrupt policy belong to the application:

- which pin is the button (`PA0`) and which is the LED (`PB15`);
- input/pull configuration;
- edge polarity (`falling`);
- IRQ priority (`15`);
- the handler symbol (`EXTI0_IRQHandler`);
- what the ISR does (STA flag) and what the main loop does (toggle LED).

There is deliberately no `Button`/`Led` alias in `device::wl55`. A generic
template such as `drivers::Led<HwPin>` is fine (it is parameterized by the
consumer), but board-specific composition is not.

## Evidence

- UM2592 Rev 4 §6.6.2: NUCLEO-WL55JC B1 → PA0 by default (SB16 ON, SB15 OFF).
- MB1389 schematic: 1 K series (R21–R23), ESDALC6V1-1U2, 100 nF (C68–C70) on
  the user-button nets.
- ST BSP `stm32wlxx-nucleo-bsp@main`:
  - `stm32wlxx_nucleo.h`: `BUTTON_SW1_PIN GPIO_PIN_0`, port `GPIOA`,
    `BUTTON_SW1_EXTI_LINE EXTI_LINE_0`, M4 `EXTI0_IRQn`.
  - `stm32wlxx_nucleo.c` `BSP_PB_Init()`: `Pull = GPIO_PULLUP`,
    `Mode = GPIO_MODE_IT_FALLING`, NVIC priority 15.
- CMSIS `stm32wl55xx.h`:
  - `EXTI_TypeDef`: RTSR1 @0x00, FTSR1 @0x04, PR1 @0x0C, IMR1 @0x80,
    C2IMR1 @0xC0 (CPU2).
  - GPIO→EXTI port routing is **SYSCFG->EXTICR[0..3]** on WL55
    (`SYSCFG_EXTICR1_EXTI0_PA/PB/PC`), not inside EXTI.
  - `EXTI0_IRQn = 6` (M4 IRQ enum).
- Vector table: `mu` assembles ST's vendored startup in place
  (`wl55jc/Startup/NOTE.md`); `startup_stm32wl55xx_cm4.s` has
  `.weak EXTI0_IRQHandler` → `Default_Handler`, so a strong app symbol
  overrides it at link time. No framework change needed.

## API

```cpp
namespace stm32::hw {

enum class Edge : std::uint8_t { Rising = 1, Falling = 2, Both = 3 };

template <class Impl> class Exti {
    template <class PortDesc, std::uint8_t Line>
    static void configure(Edge edge);
    template <std::uint8_t Line> static void enable();
    template <std::uint8_t Line> static void disable();
    template <std::uint8_t Line> static void clear_pending();
    template <std::uint8_t Line> static bool is_pending();
};

template <class Impl> class Nvic {
    template <class Irq> static void enable(Irq irq);
    template <class Irq> static void disable(Irq irq);
    template <class Irq> static void clear_pending(Irq irq);
    template <class Irq> static void set_priority(Irq irq, std::uint32_t priority);
};

}
```

WL55 implementations:

```cpp
class WlExti final : public Exti<WlExti> { /* static methods */ };
class WlNvic final : public Nvic<WlNvic> { /* CMSIS NVIC_* wrappers */ };
```

## Application composition (the user's flow)

```cpp
#include <stm32/wl5x/device/wl55/gpio.hpp>
#include <stm32/wl5x/exti.hpp>
#include <stm32/wl5x/nvic.hpp>

using LedPin    = stm32::hw::Pin<stm32::device::wl55::GpioB, 15, stm32::hw::WlClock>;
using ButtonPin = stm32::hw::Pin<stm32::device::wl55::GpioA,  0, stm32::hw::WlClock>;

void app_button_init() {
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

    LedPin::init(stm32::hw::GpioConfig{
        stm32::hw::Mode::Output,
        stm32::hw::OutputType::PushPull,
        stm32::hw::Speed::Low,
        stm32::hw::Pull::None,
        0,
    });
    LedPin::reset();
}

extern "C" void EXTI0_IRQHandler(void) {
    stm32::hw::WlExti::clear_pending<0>();
    g_button_pressed = true;   // main loop consumes and toggles LedPin
}
```

The application owns the flow; the library only exposes the pieces.

The `wl55jc` demo masks EXTI line 0 in the ISR and re-arms it 20 ms after the
main loop consumes the press. One physical press therefore produces one toggle
despite contact bounce, and holding the button does not repeat (falling edge
only).

## WL55 register mapping

| Concern | Register | Notes |
|---|---|---|
| Port routing | `SYSCFG->EXTICR[Line/4]`, shift `(Line%4)*4`, width 3 | PA=0, PB=1, PC=2 only |
| Rising trigger | `EXTI->RTSR1` | bit = line |
| Falling trigger | `EXTI->FTSR1` | bit = line |
| CPU1 interrupt mask | `EXTI->IMR1` | bit = line |
| Pending | `EXTI->PR1` | write 1 to clear |
| CPU2 mask/pending | `EXTI->C2IMR1`, `PR2` | not used in this slice |

All offsets and bits are CMSIS-derived (`offsetof`, `*_Pos`, `*_Msk`); no
hardcoded register addresses or bit numbers in the generic layer.

## Validation

- `tests/compile_check.cpp` compiles for Cortex-M4 and Cortex-M0+
  (`-DCORE_CM0PLUS`) with `-Wall -Wextra -Werror`, C++20,
  `-fno-exceptions -fno-rtti`.
- App `app_button_init()` disassembly (Cortex-M4, `-Os`) matches the intended
  writes: RCC AHB2ENR (GPIOA/GPIOB), GPIOA pull-up+input, SYSCFG EXTICR1 = PA,
  EXTI RTSR1 bit0 clear / FTSR1 bit0 set, PR1 = 1, IMR1 bit0 set,
  NVIC ICPR0/IPR6/ISER0 for IRQ 6, GPIOB output for PB15.
- Link check (`wl55jc`): `g_pfnVectors` at 0x08000000; the word at 0x58
  (`EXTI0_IRQn = 6`) points at the app's strong `EXTI0_IRQHandler`, not
  `Default_Handler`.
