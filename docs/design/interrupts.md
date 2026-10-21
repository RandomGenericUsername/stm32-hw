# Interrupts — EXTI + NVIC (WL55 first slice)

Status: implemented (library), app integration in `wl55jc`.
Date: 2026-10-20

## Scope

First external-interrupt slice for the NUCLEO-WL55JC:

- User button **B1 / BUTTON_SW1** on **PA0**.
- **Falling edge** (press) on **EXTI line 0**.
- **CPU1 / Cortex-M4**, IRQ `EXTI0_IRQn`.
- Handler symbol `EXTI0_IRQHandler`.
- ISR only clears the pending bit and sets a `volatile` flag (STA pattern);
  the main loop toggles the user LED (PB15, active high).

No software debounce in this slice. The board has a 1 K series resistor, ESD
diodes and 100 nF filter caps on the button nets, and ST's BSP relies on the
internal pull-up only.

## Evidence

- UM2592 Rev 4 §6.6.2: B1 → PA0 by default (SB16 ON, SB15 OFF).
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
  overrides it at link time. No framework change needed (validated by
  investigation).

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

WL55 implementation:

```cpp
class WlExti final : public Exti<WlExti> { /* static methods */ };
class WlNvic final : public Nvic<WlNvic> { /* CMSIS NVIC_* wrappers */ };
```

Device descriptor:

```cpp
class ButtonSw1 {
    static constexpr hw::Edge edge = hw::Edge::Falling;
    static constexpr std::uint32_t irq_priority = 15;
    static void init();          // GPIO input + pull-up + EXTI + NVIC
    static void clear_pending(); // called from the ISR
    static bool is_pressed();
};
```

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

## ISR convention

The vector table owns the symbol name. The application (not the library)
defines the strong symbol:

```cpp
extern "C" void EXTI0_IRQHandler(void) {
    stm32::device::wl55::ButtonSw1::clear_pending();
    g_button_pressed = true;
}
```

The library provides the register operations; it does not register callbacks
and does not take ownership of the IRQ symbol. This keeps the `hw` layer
stateless and lets the app choose the concurrency model.

## Validation

- `tests/compile_check.cpp` compiles for Cortex-M4 and Cortex-M0+ with
  `-Wall -Wextra -Werror`, C++20, `-fno-exceptions -fno-rtti`.
- Disassembly of `ButtonSw1::init()` (Cortex-M4, `-Os`) shows:
  - `RCC->AHB2ENR` @0x5800004C, bit 0 (GPIOAEN)
  - `GPIOA->PUPDR` pull-up, `MODER` input
  - `SYSCFG->EXTICR1` @0x40010008 field = 0 (PA)
  - `EXTI->RTSR1` bit 0 cleared, `FTSR1` bit 0 set
  - `EXTI->PR1` = 1, `EXTI->IMR1` bit 0 set
  - `NVIC->ICPR0` bit 6, `NVIC->IPR6` = 0xF0, `NVIC->ISER0` bit 6
- App link check (`wl55jc`): `g_pfnVectors` at 0x08000000; vector index 22
  (`EXTI0_IRQn` = 6) contains 0x080002f1 → the strong `EXTI0_IRQHandler` at
  0x080002f0 (Thumb bit), not `Default_Handler` at 0x08000354. The ISR writes
  1 to `EXTI->PR1` and sets the flag; `app_button_poll()` toggles PB15 via
  `BSRR`.
