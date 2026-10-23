# UART — WL55 LPUART (NUCLEO-WL55JC VCP)

Status: implemented (TX + RX slice)
Date: 2026-10-22
Target: NUCLEO-WL55JC (STM32WL55JC)

## Boundary

The library provides UART **mechanisms**:

- RCC gate for the peripheral,
- kernel-clock mux write (`RCC_CCIPR.LPUART1SEL`),
- BRR computation from the supplied kernel frequency and baud rate,
- 8N1 transmitter/receiver enable, byte write, TXE/TC status,
- RXNE status and RDR read,
- TXE/RXNE interrupt enable/disable and IRQ priority.

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
    bool receive = true;

    template <class ClockTraits>
    static constexpr UartConfig from_system(
        const BasicSystemClockConfig<ClockTraits>& sys,
        std::uint32_t baud,
        typename Traits::Source source,
        std::uint32_t lse_hz = 0u,
        bool receive = true);
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
    static std::uint8_t read_byte();
    static bool rx_ready();
    static void enable_rx_irq();
    static void disable_rx_irq();
    static bool rx_irq_pending();
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

The app derives `kernel_hz` from the selected system clock tree with
`UartConfig::from_system(...)` instead of duplicating the frequency. LSE is a
board fact and is supplied explicitly when selected.

Driver:

```cpp
template <class HwUart,
          std::size_t TxCapacity = 128,
          std::size_t RxCapacity = TxCapacity,
          class Handler = UartNullHandler>
class UartDriver {
    static bool init(const typename HwUart::Config&);
    static void write(const char*);
    static void write(const std::uint8_t*, std::size_t);
    static void write_byte(std::uint8_t); // blocks while the TX ring is full
    static bool try_write_byte(std::uint8_t); // non-blocking, ISR-safe
    static bool try_write(const char*);       // non-blocking, ISR-safe
    static void flush();                  // waits for TC
    static bool empty();
    static std::size_t pending();
    static bool read(std::uint8_t&);      // optional non-blocking RX read
    static bool rx_empty();
    static std::size_t rx_pending();
    static bool rx_overrun();
    static void clear_rx_overrun();
    static Handler& handler();            // driver-owned instance
    static bool on_irq();                 // RXNE, IDLE and TXE
};
```

`Handler` is a compile-time policy object with:

```cpp
static constexpr bool wants_idle;
void on_byte(std::uint8_t);
void on_idle();
```

The driver owns one `Handler` instance by value. There is no callback
pointer, no `void*` and no external handler lifetime.

`drivers::wl55::Lpuart1<Handler>` is:

```cpp
UartDriver<hw::WlLpuart<device::wl55::Lpuart1>, 128, 128, Handler>
```

## Interrupt ownership

`src/drivers/uart_irq_wl55.cpp` defines `LPUART1_IRQHandler` and calls
`UartIrqRouter<LPUART1_IRQn>::dispatch()`. Each driver attaches its static
`on_irq` in `init()`. The application never defines the vector and never touches
TXE or RXNE. This mirrors the ADC router split.

TX and RX each have their own single-producer/single-consumer ring:

```text
TX: application writes head_, interrupt reads tail_
RX: interrupt writes head_, application reads tail_
```

Both are `volatile`; no lock is needed on a single core. Capacity includes one
sentinel slot, so 128 slots hold 127 bytes per direction.

The ISR handles RXNE, IDLE and TXE in one entry. If the RX ring is full, it
still reads `RDR` to clear RXNE and sets `rx_overrun_`; the byte is dropped.
The application can poll `rx_overrun()`/`clear_rx_overrun()`.

RX events are delivered to the compile-time handler:

```text
Handler::on_byte(byte)   every received byte, in the UART ISR
Handler::on_idle()       USART IDLE, i.e. a frame/message boundary
```

The handler runs in interrupt context. Use `try_write`/`try_write_byte` for
any response so the ISR never blocks. The RX ring remains available for
applications that prefer `read()`.

Example handler:

```cpp
struct VcpHandler {
    static constexpr bool wants_idle = true;
    char line[64];
    std::uint32_t length = 0;

    void on_byte(std::uint8_t byte);
    void on_idle() { /* flush line */ }
};

using Vcp = stm32::drivers::wl55::Lpuart1<VcpHandler>;
```

The handler object is owned by `UartDriver<...>::handler_`, so it lives as long
as the driver. A handler member can still point at external state; that state's
lifetime remains the application's responsibility.

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
- `wl55jc` `mu build`: M4 4040 text / 16 data / 400 bss
  (128-byte TX ring + 128-byte RX ring + 64-byte line buffer + driver state).
- The compile-time handler build removes the callback pointers and registration
  code (text dropped from 5168 to 4040).
- Live loopback was captured with the previous callback-pointer build; the
  handler-policy build still needs a hardware flash when a probe is attached.
- PA3 is configured as `LPUART1_RX` in AF8; `RE`, `RXNEIE` and `IDLEIE` are
  enabled when callbacks are registered.
- `LPUART1_IRQHandler` strong at `0x08000a14`; vector IRQ 38 at
  `0x080000d8` = `0x08000a15`.
- `main` disassembly: APB1ENR2 bit 0 set, `CCIPR[11:10] = 00` (PCLK1),
  `BRR = 0x8AE4`, `CR1 = 0x00000009` (TE | UE), NVIC IP[38] = `6 << 4`.
- GPIOA pin 2 configured as alternate function 8.

## Not in this slice

- Command/protocol parsing on top of the byte/idle callbacks.
- USART1 / USART2 instances and their gates.
- DMA, flow control, parity, word lengths other than 8N1.
- Error interrupts (ORE/FE/NE); RX overrun at the ring level is tracked, but
  hardware error interrupts are not enabled.
