# Clock tree — cross-family design

Status: design proposal (not implemented)
Date: 2026-10-21
Targets: NUCLEO-WL55JC (STM32WL55JC), NUCLEO-F411RE (STM32F411RE),
NUCLEO-H755ZI-Q (STM32H755ZI), and other STM32 boards

## Problem

The current WL55 code configures the system clock with a named convenience
function:

```cpp
WlClock::use_hsi16_16mhz();
```

That works for one board and one policy, but it is not a proper abstraction:

- it encodes policy as a growing list of `use_*` functions;
- it hides the clock tree instead of describing it;
- it cannot express arbitrary source/prescaler combinations;
- it does not scale to other boards or other STM32 families;
- MSI ranges and PLL limits are device facts, not generic constants.

The clock tree must be **described as data**, validated, and then applied by
the family-specific RCC implementation.

## Goals

- Describe sources, system clock, bus prescalers, flash latency and VCORE
  range as a declarative configuration.
- Support WL55, F4, H7 and later families without pretending they have the
  same clock-tree shape.
- Keep MSI range tables, PLL limits, maximum frequencies and flash-latency
  rules in the **device** layer.
- Keep HSE/LSE frequencies and board wiring in the **board** layer.
- Keep RCC register writes in the **family clock implementation**.
- Validate illegal trees at compile time where possible.
- Make the ADC kernel clock configurable independently of the system clock.
- Keep `SystemCoreClock` correct after every clock change.

## Non-goals

- Dynamic DVFS at runtime (changing frequency while peripherals are active).
- Low-power mode entry/exit clock restoration.
- HSE clock security system (CSS).
- MSI PLL mode / LSE calibration.
- MCO and LSCO output configuration.
- Peripheral kernel-clock muxes for peripherals that do not yet have drivers
  (USART, I2C, SPI, LPTIM, ...). Those will use the same pattern later.
- Board power-tree control.

## Layers and ownership

```text
device   MCU facts: sources, MSI range table, PLL limits, max frequencies,
         flash-latency table, ADC clock limits
board    board facts: actual HSE/LSE frequency, TCXO vs crystal
hw       generic clock-tree algorithm + family-specific RCC register writes
app      selects a concrete configuration
```

The app never writes RCC/FLASH/PWR registers directly.

## Clock sources and outputs

Real sources:

| Source | Kind | Typical frequency | Can drive SYSCLK? |
|---|---|---:|---|
| MSI | internal RC | 100 kHz–48 MHz | yes |
| HSI16 | internal RC | 16 MHz | yes |
| HSE32 | external crystal/TCXO | 32 MHz | yes |
| PLL | multiplier | up to 48 MHz outputs | PLLRCLK yes |
| LSI | internal RC | ~32 kHz | no |
| LSE | external crystal | 32.768 kHz | no |

Outputs, not sources:

| Output | Purpose |
|---|---|
| MCO | outputs SYSCLK/MSI/HSI16/HSE32/PLLR/PLLP/PLLQ/LSI/LSE on PA8 |
| LSCO | outputs LSI or LSE on PA2 |

MCO and LSCO are measurement/feed outputs. They are not part of the MCU's
input clock tree.

## Cross-board model

The abstraction is:

```text
generic config + validation
        +
device ClockTraits
        +
board external-clock facts
        +
family RCC implementation
```

There is **no universal `MsiRange` enum** and **no universal config struct**.
Each device family supplies its own source type and tables. Each family clock
implementation supplies its own config shape where the clock tree differs.

## Device traits

A device family provides a `ClockTraits` type.

```cpp
namespace stm32::device::wl55 {

struct ClockTraits {
    enum class MsiRange : std::uint8_t {
        R100k, R200k, R400k, R800k, R1M, R2M,
        R4M, R8M, R16M, R24M, R32M, R48M
    };

    struct PllConfig {
        enum class Input : std::uint8_t { Msi, Hsi16, Hse32 };
        Input input;
        std::uint8_t m;   // 1..8
        std::uint8_t n;   // 6..127
        std::uint8_t r;   // 2..8
        std::uint8_t p;   // 2..32, 0 = disabled
        std::uint8_t q;   // 2..8,  0 = disabled
    };

    struct Source {
        enum class Kind : std::uint8_t { Msi, Hsi16, Hse32, Pll };
        Kind kind;
        MsiRange msi;
        PllConfig pll;
        bool hse_div2;
    };

    static constexpr std::uint32_t msi_hz(MsiRange range);
    static constexpr std::uint32_t msi_code(MsiRange range);
    static constexpr std::uint32_t source_hz(const Source& source);

    static constexpr std::uint32_t hsi16_hz = 16'000'000;
    static constexpr std::uint32_t hse_nominal_hz = 32'000'000;

    static constexpr std::uint32_t max_sysclk_hz = 48'000'000;
    static constexpr std::uint32_t max_hclk_hz = 48'000'000;
    static constexpr std::uint32_t max_pclk1_hz = 48'000'000;
    static constexpr std::uint32_t max_pclk2_hz = 48'000'000;

    static constexpr std::uint32_t flash_latency(std::uint32_t hclk_hz);
};

}
```

Other families provide their own traits:

- **F4**: no MSI, HSI/HSE/PLL, different max frequencies and latency table.
- **H7**: HSI/HSE/CSI/PLL1, D1/D2/D3 domains, different latency rules.
- **L4**: MSI with its own range table, HSI16/HSE/PLL.

A source that does not exist on a family is simply absent from that family's
`Source` type. Requesting MSI on F4 is therefore a compile error, not a
runtime surprise.

## Board traits

The board layer supplies the actual external-clock facts:

```cpp
namespace board::nucleo_wl55jc {

struct Clock : device::wl55::ClockTraits {
    static constexpr std::uint32_t hse_hz = 32'000'000; // X4 TCXO default
    static constexpr std::uint32_t lse_hz = 32'768;
};

}
```

A different board with the same MCU but a different HSE crystal only changes
this type:

```cpp
namespace board::my_wl55_board {

struct Clock : device::wl55::ClockTraits {
    static constexpr std::uint32_t hse_hz = 16'000'000;
};

}
```

No `hw` or device code changes.

## Config types

### WL55 / F4 / L4 — basic tree

These families have one AHB and two APB domains:

```cpp
template <class Traits>
struct BasicSystemClockConfig {
    typename Traits::Source source;
    AhbDivider ahb;
    ApbDivider apb1;
    ApbDivider apb2;

    constexpr std::uint32_t sysclk_hz() const {
        return Traits::source_hz(source);
    }

    constexpr std::uint32_t hclk_hz() const {
        return sysclk_hz() / to_divider(ahb);
    }

    constexpr std::uint32_t pclk1_hz() const {
        return hclk_hz() / to_divider(apb1);
    }

    constexpr std::uint32_t pclk2_hz() const {
        return hclk_hz() / to_divider(apb2);
    }

    constexpr bool valid() const;
};
```

`valid()` checks:

- SYSCLK <= `Traits::max_sysclk_hz`
- HCLK <= `Traits::max_hclk_hz`
- PCLK1 <= `Traits::max_pclk1_hz`
- PCLK2 <= `Traits::max_pclk2_hz`
- flash latency is sufficient for HCLK
- PLL input, VCO and output limits

### H7 — domain tree

H7 has D1/D2/D3 domains and PLL1/PLL2/PLL3. It uses a family-specific config:

```cpp
template <class Traits>
struct H7ClockConfig {
    typename Traits::Source source;
    D1Divider d1;
    D2Divider d2;
    D3Divider d3;
    std::uint32_t flash_latency;

    constexpr bool valid() const;
};
```

This is deliberate: forcing H7 into the WL55/F4 shape would hide real
constraints.

## Family clock implementations

The family implementation owns the register writes:

```cpp
template <class Traits>
class WlClock {
public:
    using Config = BasicSystemClockConfig<Traits>;
    static void configure(const Config& config);
};

template <class Traits>
class F4Clock {
public:
    using Config = BasicSystemClockConfig<Traits>;
    static void configure(const Config& config);
};

template <class Traits>
class H7Clock {
public:
    using Config = H7ClockConfig<Traits>;
    static void configure(const Config& config);
};
```

Common `configure()` sequence:

```text
1. validate config (compile-time where possible)
2. enable/wait for the selected source
3. program flash latency from Traits::flash_latency(hclk)
4. program AHB/APB (or D1/D2/D3) prescalers
5. select SYSCLK and wait for SWS
6. SystemCoreClockUpdate()
```

The exact register sequence is family-specific. The generic part is the
configuration, frequency math and validation.

## PLL configuration

PLL is a source, not an oscillator. It multiplies MSI, HSI16 or HSE32.

```cpp
struct PllConfig {
    PllInput input;
    std::uint8_t m;
    std::uint8_t n;
    std::uint8_t r;
    std::uint8_t p;
    std::uint8_t q;
};
```

Validation:

- PLL input 2.66–16 MHz
- VCO 96–344 MHz
- PLLR/PLLQ <= 48 MHz
- PLLP <= 48 MHz (and ADC limit when used by the ADC)

The WL55 48 MHz profile uses HSI16:

```text
HSI16 16 MHz / M=1 = 16 MHz
16 MHz × N=12 = 192 MHz VCO
192 MHz / R=4 = 48 MHz PLLRCLK
192 MHz / P=6 = 32 MHz PLLPCLK (optional ADC source)
```

## ADC clock

The ADC clock has two ownership layers:

```text
RCC:  RCC_CCIPR.ADCSEL selects None / HSI16 / PLLP / SYSCLK
ADC:  ADC_CFGR2.CKMODE selects asynchronous or PCLK/1,2,4
      ADC_CCR.PRESC divides the asynchronous clock
```

The config is data:

```cpp
template <class Traits>
struct AdcClockConfig {
    typename Traits::AdcSource source;
    typename Traits::AdcClockMode mode;
    typename Traits::AdcPrescaler prescaler;

    constexpr std::uint32_t clock_hz(
        const BasicSystemClockConfig<Traits>& sys) const;
    constexpr bool valid(
        const BasicSystemClockConfig<Traits>& sys) const;
};
```

Validation:

- ADC clock <= 35 MHz
- `PRESC` is only used in asynchronous mode
- selected source is actually available

The RCC mux is written by `WlClock`/family clock code. The ADC-local
`CKMODE`/`PRESC` are written by `WlAdc`, never by the application.

To keep `AdcDriver::init()` simple, the ADC clock config is a compile-time
policy:

```cpp
struct WlAdcClockHsi16 {
    static constexpr auto config =
        AdcClockConfig<device::wl55::ClockTraits>{
            device::wl55::ClockTraits::AdcSource::Hsi16,
            device::wl55::ClockTraits::AdcClockMode::Asynchronous,
            device::wl55::ClockTraits::AdcPrescaler::Div1,
        };
};

using Adc1 = AdcDriver<
    WlAdc<device::wl55::Adc1, SysTickDelay, WlNvic, WlAdcClockHsi16>>;
```

A different ADC clock is a different policy type. No runtime state is added
to the `hw` layer.

## Example configurations

These are values, not `use_*` functions.

### WL55

```cpp
using Clock = board::nucleo_wl55jc::Clock;

inline constexpr BasicSystemClockConfig<Clock> wl_hsi16_16mhz = {
    Clock::Source::hsi16(),
    AhbDivider::Div1,
    ApbDivider::Div1,
    ApbDivider::Div1,
};
```

### F411RE

```cpp
using Clock = board::nucleo_f411re::Clock;

inline constexpr BasicSystemClockConfig<Clock> f4_84mhz = {
    Clock::Source::pll({PllSource::Hse, 8, 336, 4, 7}),
    AhbDivider::Div1,
    ApbDivider::Div2,   // <= 50 MHz
    ApbDivider::Div1,   // <= 100 MHz
};
```

### H755

```cpp
using Clock = board::nucleo_h755zi_q::Clock;

inline constexpr H7ClockConfig<Clock> h7_480mhz = {
    Clock::Source::pll1(...),
    D1Divider::Div1,
    D2Divider::Div2,
    D3Divider::Div2,
    /* flash latency */ 4,
};
```

### Invalid config

```cpp
using F4 = board::nucleo_f411re::Clock;

// Does not compile: F4 has no MSI source.
F4::Source::msi(...);
```

## Current WL55 state

Current firmware (temporary, to be replaced by this design):

```text
SYSCLK  = HSI16 16 MHz
HCLK    = 16 MHz
PCLK1   = 16 MHz
PCLK2   = 16 MHz
SysTick = 16 MHz
ADC     = HSI16 / 1 = 16 MHz, asynchronous
```

Target after the refactor:

- same frequencies, expressed as a `BasicSystemClockConfig<BoardClock>`
- ADC clock expressed as an `AdcClockConfig<BoardClock>` policy
- no `use_*` functions in `hw`

## Migration plan

1. Write this design doc and agree on the traits/config names.
2. Add device `ClockTraits` for WL55, F4 and H7.
3. Add board traits for the existing boards.
4. Add `BasicSystemClockConfig` and `H7ClockConfig` with `valid()`.
5. Refactor `WlClock` into `WlClock<Traits>` with `configure(config)`.
6. Replace `WlClock::use_hsi16_16mhz()` with a WL55 config value.
7. Add `AdcClockConfig` and the `WlAdc` clock policy.
8. Keep F4/H7 clock implementations compile-checked but not necessarily
   hardware-validated.
9. Update `SystemCoreClock` handling and SysTick tests.

No step should change application behavior silently.

## Testing and verification

- Compile-time `static_assert(config.valid())` for every shipped config.
- Compile-check each family's clock implementation with real CMSIS headers.
- Live WL55 verification:
  - `RCC_CR`, `RCC_CFGR`, `RCC_CCIPR`, `FLASH_ACR`, `PWR_CR1`
  - `SystemCoreClock`
  - ADC `CFGR2`/`CCR`
  - SysTick frequency and loop timing
- Negative compile tests: MSI on F4, ADC clock > 35 MHz, PLL VCO out of range.
- Verify `SystemCoreClock` after each profile switch.

## Open questions

- Should board traits be generated by the project/pack, or hand-written?
- Do we expose `source_hz()`/`hclk_hz()` as runtime helpers as well as
  compile-time values?
- Do we support runtime clock changes later, or only reset-time
  configuration?
- For H7, which domain prescalers belong in the first slice?
- Do we want a generic `Clock<Traits>` facade, or keep
  `WlClock<Traits>`/`F4Clock<Traits>`/`H7Clock<Traits>` as the public names?
