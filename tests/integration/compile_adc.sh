#!/usr/bin/env sh
# Compile the three ADC family paths and their library-owned ISR sources
# against real CMSIS device headers.
#
# Required environment:
#   CMSIS_CORE  CMSIS Core Include dir (core_cm4.h / core_cm7.h)
#   CMSIS_F4    cmsis_device_f4 Include dir (stm32f4xx.h, stm32f411xe.h)
#   CMSIS_H7    cmsis_device_h7 Include dir (stm32h7xx.h, stm32h755xx.h)
#   CMSIS_WL55  cmsis_device_wl Include dir (stm32wl55xx.h)
#
# Optional: CXX (default arm-none-eabi-g++), OUT (default /tmp/stm32-hw-adc).
set -eu

: "${CMSIS_CORE:?set CMSIS_CORE to the CMSIS Core Include dir}"
: "${CMSIS_F4:?set CMSIS_F4 to the cmsis_device_f4 Include dir}"
: "${CMSIS_H7:?set CMSIS_H7 to the cmsis_device_h7 Include dir}"
: "${CMSIS_WL55:?set CMSIS_WL55 to the cmsis_device_wl Include dir}"

ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
OUT=${OUT:-/tmp/stm32-hw-adc}
CXX=${CXX:-arm-none-eabi-g++}
mkdir -p "$OUT"

COMMON="-std=c++20 -fno-exceptions -fno-rtti -mthumb -mfloat-abi=soft \
-Wall -Wextra -Werror -Os -I$CMSIS_CORE"

F4_INC="-I$ROOT/core/include -I$ROOT/f4/include"
H7_INC="-I$ROOT/core/include -I$ROOT/h7/include"
WL5X_INC="-I$ROOT/core/include -I$ROOT/wl5x/include"

"$CXX" $COMMON $F4_INC -mcpu=cortex-m4 -DSTM32F411xE -I"$CMSIS_F4" \
    -c "$ROOT/f4/tests/adc.cpp" -o "$OUT/adc_f4.o"
"$CXX" $COMMON $F4_INC -mcpu=cortex-m4 -DSTM32F411xE -I"$CMSIS_F4" \
    -c "$ROOT/f4/src/irq/adc.cpp" -o "$OUT/adc_irq_f4.o"

"$CXX" $COMMON $H7_INC -mcpu=cortex-m7 -DSTM32H755xx -DCORE_CM7 -I"$CMSIS_H7" \
    -c "$ROOT/h7/tests/adc.cpp" -o "$OUT/adc_h7.o"
"$CXX" $COMMON $H7_INC -mcpu=cortex-m7 -DSTM32H755xx -DCORE_CM7 -I"$CMSIS_H7" \
    -c "$ROOT/h7/src/irq/adc.cpp" -o "$OUT/adc_irq_h7.o"

"$CXX" $COMMON $WL5X_INC -mcpu=cortex-m4 -DSTM32WL55xx -I"$CMSIS_WL55" \
    -c "$ROOT/wl5x/tests/adc.cpp" -o "$OUT/adc_wl55.o"
"$CXX" $COMMON $WL5X_INC -mcpu=cortex-m4 -DSTM32WL55xx -I"$CMSIS_WL55" \
    -c "$ROOT/wl5x/src/irq/adc.cpp" -o "$OUT/adc_irq_wl55.o"

echo "ADC compile checks OK: $OUT"
