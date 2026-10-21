# Compile checks

## GPIO / clock / EXTI

`compile_check.cpp` covers the WL55 clock, GPIO, EXTI, NVIC and LED driver.
The consuming project (`wl55jc`) builds it as `make hw-check` for M4 and M0+.

## ADC (three families)

`compile_adc.sh` compiles one translation unit per family:

- `adc_compile_f4.cpp` — STM32F411RE, `-mcpu=cortex-m4 -DSTM32F411xE`
- `adc_compile_h7.cpp` — STM32H755ZI, `-mcpu=cortex-m7 -DSTM32H755xx -DCORE_CM7`
  (covers ADC1 and ADC3: both gate domains and both common blocks)
- `adc_compile_wl55.cpp` — STM32WL55JC, `-mcpu=cortex-m4 -DSTM32WL55xx`

Each test instantiates the ADC, the `AdcSampler` driver, the default
`SysTickDelay`/family-NVIC policies, and a completion callback with bound
arguments. The vector symbol (`ADC_IRQHandler` / `ADC3_IRQHandler`) remains
application code; it is not defined in these compile checks.

Note the delay/IRQ policies are template parameters:

```cpp
using Adc1 = stm32::hw::WlAdc<Desc, MyDelay, MyNvic>;
```

`MyDelay` only needs `init()` and `wait_us(uint32_t)`. `MyNvic` needs
`enable/disable/clear_pending/set_priority`.

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
```
