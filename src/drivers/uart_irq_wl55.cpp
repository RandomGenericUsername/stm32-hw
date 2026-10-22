// WL55 LPUART1 interrupt ownership. Compiled into the driver layer.
#include <stm32wl55xx.h>

#include "stm32/drivers/wl55.hpp"

extern "C" void LPUART1_IRQHandler(void) {
    (void)stm32::drivers::wl55::Lpuart1::on_irq();
}
