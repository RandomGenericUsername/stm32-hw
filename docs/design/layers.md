# Layer contract

The library is split so that hardware access stays stateless and peripheral
behavior lives in a driver layer.

## Layers

```text
app       board wiring, main loop, optional callbacks
drivers   stateful peripheral drivers, ISR ownership, lifecycle, callbacks
hw        stateless register mechanisms
device    MCU facts (register pointers, peripheral IDs, IRQ numbers,
          oscillator/PLL/clock facts)
board     board facts (actual HSE/LSE clocks, board-level constraints)
CMSIS     vendor headers
```

The frozen layer/ownership baseline is
[`architecture.md`](architecture.md).

## Rules

### `hw`

- Static methods only, no data members.
- No ISR symbols.
- No policy decisions (pin choice, pull, edge, priority, callback behavior).
- Operates on `device` descriptors and CMSIS register structs.

### `device`

- MCU facts only: base pointers, peripheral IDs, IRQ numbers, oscillator
  ranges, PLL limits, flash-latency rules, peripheral clock-source encodings.
- No board wiring, no application flow, no state.

### `board`

- Board facts only: actual external clocks, board-level constraints.
- Example: `board::nucleo_wl55jc::Clock` with HSE = 32 MHz TCXO.
- No application flow, no pin policy unless it becomes a reusable board
  descriptor later.

### `drivers`

- Own per-peripheral state and lifecycle.
- Own the interrupt vector symbol where the peripheral is interrupt-driven.
- Dispatch shared vectors through a fixed-size router (no heap, no virtual
  required).
- Never duplicate register logic from `hw`; call the `hw` facade.
- Expose one instance per physical peripheral (type-level singleton is the
  default).

### `app`

- Configures board pins and channels.
- Provides optional callbacks.
- Runs the main loop.
- Does **not** define ISR symbols, driver state, or dispatch tables.

## ADC as the reference

```text
hw::WlAdc<device::wl55::Adc1>   stateless register operations
drivers::wl55::Adc1             state, ISR, callback, lifecycle
ADC_IRQHandler                  compiled driver source, library-owned
app: main-M4.cpp                init/start/busy/take + optional callback
```

Shared vectors are handled by `AdcIrqRouter<IRQ>`:

- F4: ADC1/2/3 share `ADC_IRQn`.
- H7: ADC1/2 share `ADC_IRQn`; ADC3 has `ADC3_IRQn`.
- WL55: one ADC on `ADC_IRQn`.

Each driver checks its own `ISR` in `on_irq()`; the router calls all registered
drivers and ignores the ones that return false.

## Build consequence

Drivers that own vector symbols are compiled sources, one per family and per
core:

```text
src/drivers/adc_irq_f4.cpp
src/drivers/adc_irq_h7.cpp
src/drivers/adc_irq_wl55.cpp
src/drivers/uart_irq_wl55.cpp
```

The consuming project adds the matching source to its core object list. The
rest of the library remains header-only.

## UART as the second reference

```text
hw::WlLpuart<device::wl55::Lpuart1>   stateless register operations
drivers::wl55::Lpuart1                ring buffer, ISR, lifecycle
LPUART1_IRQHandler                    compiled driver source, library-owned
app: main.cpp                         pin AF, UartConfig, write/flush
```

The split is the same as the ADC: the driver owns state and the vector, the
hardware facade only touches registers, and the application supplies board
facts (pins, baud, kernel clock).
