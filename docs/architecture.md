# Architecture baseline

Status: baseline + UART RX addendum
Date: 2026-10-22
Repository layout: family-rooted (`core/`, `wl5x/`, `f4/`, `h7/`) as of
2026-10-23; the conceptual layers below are unchanged.
Baseline tag: `architecture-baseline-20261022`
Scope: STM32WL55JC / NUCLEO-WL55JC, ADC + LPUART TX/RX + clock tree

UART RX was added after the baseline tag without changing layer ownership;
the UART driver owns both directions and the ISR handles TXE and RXNE.

## Purpose

This document freezes the layer boundaries and ownership rules that the
current WL55 implementation follows. It is the reference for future
peripherals and for porting the same pattern to other families.

It is deliberately about **who owns what**, not about a specific application.

## Layers

```text
app
  Board wiring, pin configuration, clock profile selection,
  main loop, application policy, optional event handlers.

drivers
  Stateful peripheral behavior: state machines, buffers, event handlers,
  lifecycle, ISR ownership.

hw
  Stateless register mechanisms. Static methods only. No state.
  Family-specific facades: WlClock, WlAdc, WlLpuart, Pin, Nvic, Exti.

device
  MCU facts: register pointers, peripheral IDs, IRQ numbers,
  oscillator range tables, PLL limits, flash-latency rules,
  peripheral clock-source encodings.

board
  Board facts: actual external clocks and board-level constraints.
  Example: NUCLEO-WL55JC HSE = 32 MHz TCXO, LSE = 32.768 kHz.

CMSIS
  Vendor headers.
```

`board` is facts, not behavior. Physical pin wiring remains application policy
unless it becomes a reusable board descriptor later.

## Dependency direction

```text
app
  -> drivers
  -> hw
  -> device
  -> board facts
  -> CMSIS

drivers
  -> hw
  -> device

hw
  -> device
  -> CMSIS

device
  -> CMSIS

board
  -> device
```

`hw` never includes `drivers` or `app`.
`device` never includes `hw` behavior or `drivers`.
`board` never includes `app` or `drivers`.

## Ownership matrix

| Concern | Owner |
|---|---|
| Oscillator enable/ready | `hw::WlClock<Traits>` |
| System clock source mux | `hw::WlClock<Traits>` |
| AHB/APB prescalers | `hw::WlClock<Traits>` |
| Flash latency / VCORE range | `hw::WlClock<Traits>` |
| System clock facts and limits | `device::wl55::ClockTraits` |
| Board HSE/LSE facts | `board::nucleo_wl55jc::Clock` |
| Declarative system clock tree | `hw::BasicSystemClockConfig<Traits>` |
| `RCC_CCIPR.ADCSEL` | `hw::WlClock<Traits>` |
| ADC `CKMODE` / `PRESC` | `hw::WlAdc` |
| ADC clock facts and 35 MHz limit | `device::wl55::AdcClockTraits` |
| ADC state / handler | `drivers::AdcDriver` |
| ADC ISR | `wl5x/src/irq/adc.cpp` |
| `RCC_CCIPR.LPUART1SEL` | `hw::WlLpuart` via `hw::WlClock` |
| UART BRR / 8N1 TX | `hw::WlLpuart` |
| UART clock-source encodings | `device::wl55::UartClockTraits` |
| UART TX/RX rings / ISR state | `drivers::UartDriver` |
| UART ISR (TXE + RXNE) | `wl5x/src/irq/uart.cpp` |
| GPIO mode / AF / pull | app using `hw::Pin` |
| IRQ priority policy | app, passed to the driver |
| Source list / library objects | `config/sources.mk` |

## Clock split

System clock:

```text
device::wl55::ClockTraits
    MSI ranges, PLL limits, max frequencies, flash latency

board::nucleo_wl55jc::Clock
    HSE 32 MHz TCXO, LSE 32.768 kHz

BasicSystemClockConfig<Clock>
    source + AHB/APB1/APB2 dividers

WlClock<Clock>::configure(config)
    writes RCC/FLASH/PWR, updates SystemCoreClock
```

Peripheral clock facts are separate:

```text
device::wl55::AdcClockTraits
    Source, Mode, Prescaler, 35 MHz limit

device::wl55::UartClockTraits
    Pclk1 / Sysclk / Hsi16 / Lse encodings
```

The clock layer only writes the RCC mux fields:

```text
WlClock::select_adc_clock(std::uint32_t)
WlClock::select_lpuart1_clock(std::uint32_t)
```

The peripheral layer owns its own clock registers:

```text
WlAdc      -> ADC_CFGR2.CKMODE, ADC_CCR.PRESC
WlLpuart   -> BRR, CR1/CR2/CR3
```

This is intentional. `ADCSEL` and `LPUART1SEL` live in RCC; `CKMODE` and
`PRESC` live in the ADC. The raw `std::uint32_t` at the clock-layer boundary
is the deliberate decoupling point; the typed enum stays in `device`.

## ISR ownership

The application never defines an ISR for a library-owned peripheral.

```text
ADC:
    ADC_IRQHandler
      -> AdcIrqRouter<IRQ>::dispatch()
        -> AdcDriver::on_irq()
          -> Handler::on_complete(value)

UART:
    LPUART1_IRQHandler
      -> UartDriver::on_irq()
        -> RXNE: read RDR into the RX ring (or drop + overrun flag)
                 and call Handler::on_byte(value)
        -> IDLE: clear IDLE and call Handler::on_idle()
        -> TXE:  write the next TX ring byte
```

The vector symbols are compiled sources, not header-inline code:

```text
wl5x/src/irq/adc.cpp
wl5x/src/irq/uart.cpp
```

The consuming project adds them through `config/sources.mk`.

## Driver state

`AdcDriver` owns:

```text
busy_, ready_, value_, handler_
```

`UartDriver` owns:

```text
TX ring buffer + indices
RX ring buffer + indices
RX overrun flag
compile-time Handler (on_byte, on_idle)
```

Both are type-level singletons, matching the one-physical-peripheral model.
No heap, exceptions or RTTI are used.

## Application boundary

The app owns:

- which pins and alternate functions
- which clock profile
- baud rate and UART kernel source
- ADC channel and sampling time
- what to print or do with samples
- IRQ priorities
- optional compile-time event handlers, constrained by the `AdcHandler` /
  `UartHandler` concepts

The app does not own:

- ADC EOC clearing
- UART TXE handling
- RCC source/prescaler register writes
- flash-latency programming
- ISR vector symbols

## Build boundary

The generated Makefile is owned by mu. Do not hand-edit it.

Library sources and include paths go through `config/sources.mk`:

```make
EXTRA_SRCS_M4 = third_party/stm32-hw/wl5x/src/irq/adc.cpp \
                third_party/stm32-hw/wl5x/src/irq/uart.cpp
EXTRA_INC_M4  = -Ithird_party/stm32-hw/core/include \
                -Ithird_party/stm32-hw/wl5x/include \
                -Ithird_party/stm32-hw/wl5x/boards/nucleo_wl55jc/include
```

`compile_commands.json` is generated by the build for editor IntelliSense.

## Baseline invariants

- `hw` has no data members and no ISR symbols.
- `drivers` own state and interrupt vectors.
- `device` contains MCU facts only.
- `board` contains board facts only.
- The app never defines `ADC_IRQHandler` or `LPUART1_IRQHandler`.
- The app never clears ADC EOC or UART TXE.
- No heap, exceptions or RTTI.
- Generated Makefile is not edited by hand.
- `SystemCoreClock` is updated by the clock layer.
- ADC clock and system clock are validated separately.

## Deferred

- F4/H7 `ClockTraits` and family clock implementations.
- H7 D1/D2/D3 clock-domain configuration.
- UART command/protocol parser on top of Handler::on_byte/on_idle.
- USART1/USART2 descriptors and gates.
- Runtime clock switching / DVFS.
- MCO/LSCO configuration.
- Board-level reusable pin descriptors.
- Additional peripheral kernel-clock muxes (I2C, SPI, LPTIM, ...).
