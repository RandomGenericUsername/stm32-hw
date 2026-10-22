#!/usr/bin/env sh
# Compile the WL55 LPUART driver path and its library-owned ISR source
# against real CMSIS device headers.
#
# Required environment:
#   CMSIS_CORE  CMSIS Core Include dir (core_cm4.h)
#   CMSIS_WL55  cmsis_device_wl Include dir (stm32wl55xx.h)
#
# Optional: CXX (default arm-none-eabi-g++), OUT (default /tmp/stm32-hw-uart).
set -eu

: "${CMSIS_CORE:?set CMSIS_CORE to the CMSIS Core Include dir}"
: "${CMSIS_WL55:?set CMSIS_WL55 to the cmsis_device_wl Include dir}"

ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
OUT=${OUT:-/tmp/stm32-hw-uart}
CXX=${CXX:-arm-none-eabi-g++}
mkdir -p "$OUT"

COMMON="-std=c++20 -fno-exceptions -fno-rtti -mthumb -mfloat-abi=soft \
-Wall -Wextra -Werror -Os -I$ROOT/include -I$CMSIS_CORE"

"$CXX" $COMMON -mcpu=cortex-m4 -DSTM32WL55xx -I"$CMSIS_WL55" \
    -c "$ROOT/tests/uart_compile_wl55.cpp" -o "$OUT/uart_wl55.o"
"$CXX" $COMMON -mcpu=cortex-m4 -DSTM32WL55xx -I"$CMSIS_WL55" \
    -c "$ROOT/src/drivers/uart_irq_wl55.cpp" -o "$OUT/uart_irq_wl55.o"

echo "UART compile checks OK: $OUT"
