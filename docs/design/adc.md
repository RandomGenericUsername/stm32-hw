# ADC — cross-family design (F4 / H7 / WL55)

Status: design proposal (not implemented)
Date: 2026-10-20
Targets: NUCLEO-F411RE (STM32F411RE), NUCLEO-H755ZI-Q (STM32H755ZI),
NUCLEO-WL55JC (STM32WL55JC)

## Boundary

The library provides ADC **mechanisms**:

- bus gate enable (RCC),
- ADC kernel-clock source/divider selection,
- instance power-up, regulator, calibration, ready wait,
- channel selection, sample time, resolution,
- one software-triggered conversion, polled result.

The application owns board policy:

- which pin/channel is measured,
- pin analog mode,
- sample time choice and resolution,
- what to do with the value (filter, threshold, telemetry).

No `NucleoAdc`/`BoardAdc` class in the library. Device descriptors contain
only CMSIS pointers/IDs.

## Why three family implementations

The three MCUs use three different ADC IP layouts, not one layout with
different addresses:

| | F411RE | H755 | WL55JC |
|---|---|---|---|
| Instances | ADC1 | ADC1, ADC2, ADC3 | ADC1 |
| IP family | F4 legacy | H7 | L4-style |
| Base regs | SR, CR1, CR2, SMPR1/2, SQR1-3, DR | ISR, IER, CR, CFGR, CFGR2, SMPR1/2, PCSEL, SQR1-4, DR | ISR, IER, CR, CFGR1, CFGR2, SMPR, CHSELR, DR |
| Bus gate | `RCC_APB2ENR` bit 8 | `RCC_AHB1ENR` bit 5 (ADC12); `RCC_AHB4ENR` bit 24 (ADC3) | `RCC_APB2ENR` bit 9 |
| Kernel clock | PCLK2, `ADC_CCR.ADCPRE` (/2 at reset) | `RCC_D3CCIPR.ADCSEL` = CLKP, `ADC_CCR.PRESC` | `RCC_CCIPR.ADCSEL` = HSI16, `ADC_CFGR2.CKMODE` (async) + `ADC_CCR.PRESC` |
| Enable | `ADON` + tSTAB (~3 µs) | `ADVREGEN` (10 µs) → `ADCAL` → `ADEN` → `ADRDY` | `ADVREGEN` (20 µs) → `ADCAL` → `ADEN` → `ADRDY` |
| Channel select | `SQR3.SQ1`, `SQR1.L=0` | `SQR1.SQ1`, `SQR1.L=0` | `CHSELR` bit, wait `CCRDY` |
| Sample time | `SMPR2` ch0-9, `SMPR1` ch10-18 | `SMPR1` ch0-9, `SMPR2` ch10-19 | `SMP1/SMP2` + `SMPSELx` per channel |
| Resolution | 12/10/8/6 | 16/14/12/10/8 | 12/10/8/6 |
| Result | `DR` | `DR` | `DR` |

Evidence:

- F4: `stm32f411xe.h` ADC struct @152-174, `ADC1_BASE` @678,
  `RCC_APB2ENR_ADC1EN_Pos` @4366, `ADC_CCR_ADCPRE_Pos` @1369.
- H7: `stm32h755xx.h` ADC struct @265-309, `ADC1_BASE` @2274,
  `RCC_AHB1ENR_ADC12EN_Pos` @16205, `RCC_AHB4ENR_ADC3EN_Pos` @16319,
  `RCC_D3CCIPR_ADCSEL_Pos` @15999, `ADC_CCR_PRESC_Pos` @4143.
- WL55: local `stm32wl55xx.h` ADC struct @232-253, `ADC_BASE` @985,
  `ADC_COMMON_BASE` @986, `RCC_APB2ENR_ADCEN_Pos` @7196,
  `RCC_CCIPR_ADCSEL_Pos` @7397, `ADC_CFGR2_CKMODE_Pos` @1399.
- WL55 RM0453 §18.3.5 (ADC clock), §18.3.9 (SMP1/SMP2 + SMPSEL),
  §18.12.6 (SMPR).
- ST LL/HAL sequences: `stm32f4xx_ll_adc.h` `LL_ADC_Enable`/SWStart;
  `stm32h7xx_ll_adc.h` `LL_ADC_Enable`/`StartCalibration`/BOOST modes;
  `stm32wlxx_ll_adc.h` `LL_ADC_Enable`/`StartCalibration`.

## API

```cpp
namespace stm32::hw {

template <class Impl>
class Adc {
public:
    static void init();          // gate + kernel clock + regulator + calibration
    static void disable();

    template <std::uint8_t Channel, class SampleTime>
    static void configure(SampleTime sample_time);

    static void start();         // enable EOC IRQ + software trigger
    static bool irq_pending();
    static std::uint16_t read_result(); // clear EOC, return DR
    static void enable_irq();
    static void disable_irq();
    static void set_irq_priority(std::uint32_t priority);

    template <std::uint8_t Channel>
    static std::uint16_t read(); // explicit blocking convenience
};

}
```

Family classes take the delay and NVIC policies:

```cpp
template <class Desc, class Delay = SysTickDelay, class Irq = F4Nvic>
class F4Adc;

template <class Desc, class Delay = SysTickDelay, class Irq = H7Nvic>
class H7Adc;

template <class Desc, class Delay = SysTickDelay, class Irq = WlNvic>
class WlAdc;
```

`Delay` supplies `init()` and `wait_us(uint32_t)`. `Irq` supplies
`enable/disable/clear_pending/set_priority`. Both are compile-time policies, so
an application can inject its own timer/tick or NVIC wrapper without changing
the ADC driver. `SysTickDelay` is the reference implementation: a polled
free-running 24-bit SysTick down-counter at the core clock, with the interrupt
disabled. `wait_us()` converts microseconds to core ticks and chunks long
delays to stay inside the 24-bit period. (A 1 MHz reload is unsafe on low-clock
parts: at 4 MHz the reload is only 4 ticks and the modulo-24 difference wraps
before it reaches the reload.)

Family sample-time enums (the codes are the real 3-bit fields):

```cpp
// F4:   3, 15, 28, 56, 84, 112, 144, 480 ADC cycles
// H7:   1.5, 2.5, 6.5, 12.5, 24.5, 47.5, 92.5, 247.5
// WL55: 1.5, 3.5, 7.5, 12.5, 19.5, 39.5, 79.5, 160.5
```

They are deliberately per-family: forcing one normalized enum would silently
round timing.

## Layout

```text
include/stm32/hw/adc.hpp              # Adc<Impl> facade
include/stm32/hw/adc/f4.hpp           # F4Adc<Desc>
include/stm32/hw/adc/h7.hpp           # H7Adc<Desc>
include/stm32/hw/adc/wl55.hpp         # WlAdc<Desc>

include/stm32/device/f4/adc.hpp       # Adc1 descriptor
include/stm32/device/h7/adc.hpp       # Adc1/2/3 descriptors
include/stm32/device/wl55/adc.hpp     # Adc1 descriptor

include/stm32/drivers/adc.hpp         # AdcDriver<HwAdc, Handler> (state + policy)
include/stm32/drivers/adc_irq.hpp     # AdcIrqRouter<Irq> (shared vectors)
include/stm32/drivers/f4.hpp          # drivers::f4::Adc1
include/stm32/drivers/h7.hpp          # drivers::h7::Adc1/2/3
include/stm32/drivers/wl55.hpp        # drivers::wl55::Adc1

src/drivers/adc_irq_f4.cpp            # library-owned ADC_IRQHandler
src/drivers/adc_irq_h7.cpp            # ADC_IRQHandler + ADC3_IRQHandler
src/drivers/adc_irq_wl55.cpp          # library-owned ADC_IRQHandler
```

Clock classes gain the gate + kernel source:

- `F4Clock`: `F4Gate<Peripheral::Adc1>`; no source select (PCLK2).
- `H7Clock`: `H7Gate<Adc1/2/3>`; `select_adc_clock<ClkP>()` → `RCC_D3CCIPR`.
- `WlClock`: `WlGate<Adc1>`; `select_adc_clock<Hsi16>()` → `RCC_CCIPR`.

The ADC family implementation sets the divider that lives in the ADC block
(`F4 ADC_CCR.ADCPRE`, `H7/WL55 ADC_CCR.PRESC`, `WL55 CFGR2.CKMODE`).

## Driver layer

`AdcDriver<HwAdc, Handler>` is a type-level, stateful driver. `HwAdc` is the
stateless family facade. `Handler` is a C++20 concept-constrained, compile-time
policy with `void on_complete(std::uint16_t)`, called in the ADC interrupt.

```cpp
template <class H>
concept AdcHandler =
    std::default_initializable<H> &&
    requires(H& handler, std::uint16_t value) {
        { handler.on_complete(value) } -> std::same_as<void>;
    };

template <class HwAdc, AdcHandler Handler = AdcNullHandler>
class AdcDriver {
public:
    using SampleTime = typename HwAdc::SampleTime;

    static void init();
    static void stop();

    template <std::uint8_t Channel>
    static void start(SampleTime sample_time);

    static bool busy();
    static bool ready();
    static std::uint16_t take();

    static void set_irq_priority(std::uint32_t priority);
    static Handler& handler();

private:
    static bool on_irq();   // called by the library ISR/router

    static inline volatile bool busy_ = false;
    static inline volatile bool ready_ = false;
    static inline volatile std::uint16_t value_ = 0;
    static inline Handler handler_{};
};
```

The driver owns:

- conversion state (`busy_`, `ready_`, `value_`)
- the handler object by value
- the EOC/DR/IRQ handling
- registration with the IRQ router

There is no callback pointer and no external handler lifetime.

The application never defines `ADC_IRQHandler`.

## WL55 VREFINT calibration

The internal reference is sampled on ADC VIN[13]. Converting that raw sample
to a calibrated voltage uses the factory count in system memory:

```text
VREF+ = VREF+_Charac × VREFINT_CAL / VREFINT_DATA     (RM0453 §18.9)
```

- `device::wl55::Adc1::vrefint_cal_address` = `0x1FFF75AA` (16-bit,
  DS13293 §3.20.2 Table 14: acquired at VDDA = VREF+ = 3.3 V ±10 mV, 30 °C).
- `device::wl55::Adc1::vrefplus_charac_mv` = `3300`.
- `hw::WlAdc<...>::vrefint_cal()` reads the count (stateless memory access).
- `drivers::wl55::Adc1::vrefplus_millivolts(raw)` applies the formula with
  rounding; `vrefplus_millivolts(cal, raw)` is the pure, compile-time-checked
  form.

This returns the calibrated **VREF+ (analog supply/reference)** in millivolts,
not the ~1.2 V bandgap voltage itself.

## Library-owned ISR

Each family has a compiled driver source:

```cpp
// src/drivers/adc_irq_wl55.cpp
extern "C" void ADC_IRQHandler(void)
{
    stm32::drivers::AdcIrqRouter<ADC_IRQn>::dispatch();
}
```

- WL55: one ADC, one router.
- F4: ADC1/2/3 share `ADC_IRQn`; the router dispatches to every registered
  driver, and each `on_irq()` checks its own `ISR`.
- H7: ADC1/2 share `ADC_IRQn`; ADC3 has `ADC3_IRQn` and its own router.

The router is a fixed-size static array of `bool (*)()` handlers. No heap,
no virtual, no user registration.

## Usage (application side)

```cpp
#include <stm32/drivers/wl55.hpp>

using Adc1 = stm32::drivers::wl55::Adc1;

struct AdcHandler {
    void on_complete(std::uint16_t value) {
        // runs in the ADC interrupt
        (void)value;
    }
};

using Adc1 = stm32::drivers::wl55::Adc1<AdcHandler>;

Adc1::init();
Adc1::enable_vrefint();       // WL55 internal channel VIN[13]
Adc1::set_irq_priority(5);

for (;;) {
    Adc1::start<13>(Adc1::SampleTime::Cycles160_5);

    while (Adc1::busy()) {
    }

    const std::uint16_t value = Adc1::take();
    const std::uint32_t vrefplus_mv = Adc1::vrefplus_millivolts(value);
    (void)vrefplus_mv;
    // delay
}
```

No user ISR, no sampler object, no state struct, no globals. The driver owns the
state and the handler. With `AdcNullHandler`, `ready()`/`take()` remain
sufficient; the handler is a compile-time policy, not a runtime callback.

For an external channel, the app also configures the pin as analog:

```cpp
using Pin = stm32::hw::Pin<stm32::device::wl55::GpioA, 0, stm32::hw::WlClock>;
Pin::init({ hw::Mode::Analog, hw::OutputType::PushPull,
            hw::Speed::Low, hw::Pull::None, 0 });
```

The pin↔channel mapping is board knowledge and stays in the app.

## Interrupt path

`AdcDriver::start()` enables `EOCIE` and the software trigger. The vector
symbol is owned by the compiled driver source:

```text
ADC EOC
  -> ADC_IRQHandler              (src/drivers/adc_irq_*.cpp)
    -> AdcIrqRouter<IRQ>::dispatch()
      -> AdcDriver<HwAdc>::on_irq()
        -> read DR / clear EOC / busy=false / ready=true
        -> Handler::on_complete(value)
```

F4 ADC1/2/3 and H7 ADC1/2 share a vector; each registered driver checks its own
`ISR` and returns false when the interrupt is not its own. H7 ADC3 uses its own
`ADC3_IRQHandler`. The app never defines an ISR.

## First slice

In scope:

- one channel, one conversion, software trigger;
- interrupt-driven completion (`EOC`) owned by the driver;
- driver-owned state: `busy`, `ready`, `value`;
- C++20 `AdcHandler` policy (`Handler::on_complete(std::uint16_t)`),
  driver-owned;
- library-owned ISR with a fixed-size router for shared vectors;
- fixed 12-bit right-aligned on all three (no resolution argument yet;
  H7 16/14-bit is deferred);
- family `init()` includes regulator/calibration/ready;
- delay and NVIC injected as policies (`SysTickDelay` is the reference);
- three compile checks plus the three ISR sources with the real CMSIS headers;
- disassembly checks for gate/clock/init/start/EOCIE/NVIC per family.

Out of scope:

- DMA, scan/continuous, injected, analog watchdog, oversampling,
  differential, multimode;
- calibration-factor save/restore;
- channel-to-pin tables (app/board data);
- F4/H7 `Pin`/`GPIO` device descriptors unless needed by examples.

## Defaults

| | default init |
|---|---|
| F4 | APB2 gate; PCLK2/2 (8 MHz at 16 MHz HSI); ADON + 3 µs |
| H7 | AHB1/AHB4 gate; CLKP (per_ck) /4 (16 MHz at 64 MHz HSI); BOOST 20 MHz; regulator 10 µs; calibrate; ADEN |
| WL55 | APB2 gate; HSI16 enabled/ready then selected as kernel clock; HSI16 /1 (16 MHz); async CKMODE; regulator 20 µs; calibrate; ADEN |

## WL55 clock tree

The consumer configures the system clock explicitly before enabling the ADC
and the SysTick delay:

```cpp
stm32::hw::WlClock::use_hsi16_16mhz();
```

That gives:

```text
HSI16 16 MHz -> SYSCLK -> HCLK  = 16 MHz  (AHB  /1)
                        -> PCLK1 = 16 MHz  (APB1 /1)
                        -> PCLK2 = 16 MHz  (APB2 /1)
SysTick (core clock)    = 16 MHz
Flash latency           = 0 WS (VCORE range 1, RM0453 Table 12)
ADC kernel clock        = HSI16 / 1 = 16 MHz, asynchronous
                          (RCC_CCIPR.ADCSEL = HSI16,
                           ADC_CFGR2.CKMODE = 00,
                           ADC_CCR.PRESC = 0000)
```

`WlClock::SysclkSource` uses the real RCC_CFGR encodings from RM0453 §7.4.3:
`MSI = 00`, `HSI16 = 01`, `HSE32 = 10`, `PLL = 11`. Do not assume
`HSI16 = 10`; that selects HSE32 and the switch never completes if HSE32 is
not running.

## Verification plan

1. Compile checks:
   - F4: `-mcpu=cortex-m4 -DSTM32F411xE`, F4 CMSIS include.
   - H7: `-mcpu=cortex-m7 -DSTM32H755xx -DCORE_CM7`, H7 CMSIS include.
   - WL55: `-mcpu=cortex-m4 -DSTM32WL55xx`, vendored WL55 include.
   - Each family's `src/drivers/adc_irq_*.cpp` compiles beside its driver TU.
2. Link/vector proof in the consumer:
   - `ADC_IRQHandler` is the strong library symbol, not the weak startup stub.
   - the vector entry for `ADC_IRQn` points at it.
3. Disassembly:
   - F4: `RCC_APB2ENR` bit 8, `ADC_CCR.ADCPRE`, `CR2.ADON`, `SQR3/SMPR2`,
     `CR2.SWSTART`, `SR.EOC`, `DR`.
   - H7: `RCC_AHB1ENR` bit 5, `D3CCIPR.ADCSEL`, `ADC_CCR.PRESC`,
     `CR.ADVREGEN/ADCAL/ADEN`, `CFGR`, `SQR1/SMPR`, `CR.ADSTART`,
     `ISR.EOC`, `DR`.
   - WL55: `RCC_APB2ENR` bit 9, `CCIPR.ADCSEL`, `CFGR2.CKMODE`,
     `CR.ADVREGEN/ADCAL/ADEN`, `CFGR1`, `CHSELR/SMPR`, `CR.ADSTART`,
     `ISR.EOC`, `DR`.
3. Optional live WL55 read of VREFINT (no external wiring) to prove the
   sequence on hardware; F4/H7 compile-only unless a board is attached.
