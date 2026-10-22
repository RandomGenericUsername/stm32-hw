# Compile checks

## GPIO / clock / EXTI

`compile_check.cpp` covers the WL55 clock, GPIO, EXTI, NVIC and LED driver.
The consuming project (`wl55jc`) builds it as `make hw-check` for M4 and M0+.

## ADC (three families)

`compile_adc.sh` compiles one driver translation unit per family plus the
library-owned ISR source:

- `adc_compile_f4.cpp` — STM32F411RE, `-mcpu=cortex-m4 -DSTM32F411xE`
- `adc_compile_h7.cpp` — STM32H755ZI, `-mcpu=cortex-m7 -DSTM32H755xx -DCORE_CM7`
  (covers ADC1 and ADC3: both gate domains and both common blocks)
- `adc_compile_wl55.cpp` — STM32WL55JC, `-mcpu=cortex-m4 -DSTM32WL55xx`
- `src/drivers/adc_irq_f4.cpp`, `..._h7.cpp`, `..._wl55.cpp`

Each test uses the type-level driver (`drivers::f4::Adc1`, `drivers::h7::Adc1/3`,
`drivers::wl55::Adc1`), the default `SysTickDelay`/family-NVIC policies, and an
optional free-function completion callback. The vector symbol
(`ADC_IRQHandler` / `ADC3_IRQHandler`) is defined by the library ISR source, not
by the application.

Note the delay/IRQ policies are template parameters of the hardware facade:

```cpp
using Adc1 = stm32::drivers::f4::Adc1; // AdcDriver<F4Adc<...>>
```

`MyDelay` only needs `init()` and `wait_us(uint32_t)`. `MyNvic` needs
`enable/disable/clear_pending/set_priority`.

## UART (WL55 LPUART)

`compile_uart.sh` compiles the WL55 LPUART driver path and its library-owned
ISR source:

- `uart_compile_wl55.cpp` — STM32WL55JC, `-mcpu=cortex-m4 -DSTM32WL55xx`
- `src/drivers/uart_irq_wl55.cpp` — defines `LPUART1_IRQHandler`

```sh
export CMSIS_CORE=/path/to/CMSIS/Core/Include
export CMSIS_WL55=/path/to/cmsis_device_wl/Include
./tests/compile_uart.sh
```

It uses `drivers::wl55::Lpuart1`, the default `WlNvic` policy, a `UartConfig`
supplied by the caller, and `static_assert`s on `brr_for()`.

The library does not vendor CMSIS. Point the script at the device headers from
the consuming projects:

```sh
export CMSIS_CORE=/path/to/CMSIS/Core/Include
export CMSIS_F4=/path/to/cmsis_device_f4/Include
export CMSIS_H7=/path/to/cmsis_device_h7/Include
export CMSIS_WL55=/path/to/cmsis_device_wl/Include
./tests/compile_adc.sh
```

It compiles with `-Wall -Wextra -Werror`, C++20, `-fno-exceptions -fno-rtti`.
For disassembly evidence, inspect the produced objects:

```sh
arm-none-eabi-objdump -d /tmp/stm32-hw-adc/adc_f4.o
arm-none-eabi-objdump -d /tmp/stm32-hw-adc/adc_h7.o
arm-none-eabi-objdump -d /tmp/stm32-hw-adc/adc_wl55.o
arm-none-eabi-objdump -d /tmp/stm32-hw-adc/adc_irq_wl55.o
```
