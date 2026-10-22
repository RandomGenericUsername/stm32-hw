# UART — WL55 LPUART (NUCLEO-WL55JC VCP)

Status: implemented (TX slice)
Date: 2026-10-22
Target: NUCLEO-WL55JC (STM32WL55JC)

## Boundary

The library provides UART **mechanisms**:

- RCC gate for the peripheral,
- kernel-clock mux write (`RCC_CCIPR.LPUART1SEL`),
- BRR computation from the supplied kernel frequency and baud rate,
- 8N1 transmitter enable, byte write, TXE/TC status,
- TXE interrupt enable/disable and IRQ priority.

The application owns board policy:

- which instance and pins (LPUART1 on PA2/PA3 for the VCP),
- alternate function, pull and speed,
- baud rate and kernel frequency/source,
- what to send and when.

No board alias lives in the library. `device::wl55::Lpuart1` contains only the
CMSIS register pointer, the peripheral id and the IRQ line.

## Why LPUART1 first

UM2592 §6.6.5: on the stock NUCLEO-WL55JC the ST-LINK-V3E virtual COM port is
wired to **LPUART1 on PA2/PA3** (SB3/SB5 ON). USART1 on PB6/PB7 is wired to the
Arduino header by default. The first slice therefore covers LPUART1 TX only,
which is what a serial monitor needs.

`PA2 = LPUART1_TX` on **AF8** (DS13293 Table 21, §3.16).

## API

```cpp
namespace stm32::hw {

template <class Traits>
struct UartConfig {
    std::uint32_t baud;
    std::uint32_t kernel_hz;
    typename Traits::Source source;
};

template <class Impl, class Traits>
class Uart {
    using Config = UartConfig<Traits>;
    static bool init(const Config&);       // false if BRR is out of range
    static void disable();
    static void write_byte(std::uint8_t);
    static bool tx_ready();
    static bool tx_complete();
    static void enable_tx_irq();
    static void disable_tx_irq();
    static bool tx_irq_pending();
    static void set_irq_priority(std::uint32_t);
    static constexpr auto irq();
};

template <class Desc, class Irq = WlNvic>
class WlLpuart final
    : public Uart<WlLpuart<Desc, Irq>, typename Desc::UartClockTraits> {
    static constexpr std::uint32_t brr_for(std::uint32_t kernel_hz,
                                           std::uint32_t baud);
};

} // namespace stm32::hw
```

The source encoding is an MCU fact in
`device::wl55::UartClockTraits` (`Pclk1`, `Sysclk`, `Hsi16`, `Lse`), matching
the `AdcClockTraits` split. The generic `hw` layer only carries the value.

Driver:

```cpp
template <class HwUart, std::size_t Capacity = 128>
class UartDriver {
    static bool init(const typename HwUart::Config&);
    static void write(const char*);
    static void write(const std::uint8_t*, std::size_t);
    static void write_byte(std::uint8_t); // blocks while the ring is full
    static void flush();                  // waits for TC
    static bool empty();
    static std::size_t pending();
    static bool on_irq();                 // called by the library ISR
};
```

`drivers::wl55::Lpuart1` is `UartDriver<hw::WlLpuart<device::wl55::Lpuart1>>`.

## Interrupt ownership

`src/drivers/uart_irq_wl55.cpp` defines `LPUART1_IRQHandler` and calls
`drivers::wl55::Lpuart1::on_irq()`. The application never defines the vector
and never touches TXE. This mirrors the ADC split.

The ring buffer is single-producer/single-consumer: `head_` is written only by
thread context, `tail_` only by the interrupt. Both are `volatile`; no lock is
needed on a single core. Capacity includes one sentinel slot, so 128 slots hold
127 bytes.

## BRR

RM0453 §30.5.4 (LPUART):

```text
baud = 256 × f_ck / BRR        BRR = 256 × f_ck / baud
```

BRR is 16-bit and must be ≥ `0x300`. At 16 MHz the valid baud window is
roughly 62.5 kbit/s … 5.3 Mbit/s, so 115200 is fine but 9600 is not
representable. `init()` returns `false` instead of programming a bad divider.
Low baud rates need a slower kernel clock (LSE is selectable in `UartConfig`).

At PCLK1 = 16 MHz and 115200 baud: `BRR = 35556 (0x8AE4)`.

## Validation

- `tests/compile_uart.sh` compiles the driver and the ISR source for
  Cortex-M4 / STM32WL55xx with `-Wall -Wextra -Werror`, C++20,
  `-fno-exceptions -fno-rtti`.
- `static_assert` covers `brr_for(16 MHz, 115200) == 35556`,
  `brr_for(32 MHz, 115200) == 71111`, `brr_for(LSE, 9600) == 874` and the
  zero-baud guard.
- `wl55jc` `mu build`: M4 3676 text / 16 data / 184 bss
  (128-byte TX ring + driver state).
- `LPUART1_IRQHandler` strong at `0x08000a14`; vector IRQ 38 at
  `0x080000d8` = `0x08000a15`.
- `main` disassembly: APB1ENR2 bit 0 set, `CCIPR[11:10] = 00` (PCLK1),
  `BRR = 0x8AE4`, `CR1 = 0x00000009` (TE | UE), NVIC IP[38] = `6 << 4`.
- GPIOA pin 2 configured as alternate function 8.

## Not in this slice

- RX / receive ring buffer and callbacks.
- USART1 / USART2 instances and their gates.
- DMA, flow control, parity, word lengths other than 8N1.
- Error interrupts (ORE/FE/NE).
