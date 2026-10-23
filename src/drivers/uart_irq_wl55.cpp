// WL55 LPUART1 interrupt ownership. Compiled into the driver layer.
#include <stm32wl55xx.h>

#include "stm32/drivers/uart_irq.hpp"

extern "C" void LPUART1_IRQHandler(void) {
    stm32::drivers::UartIrqRouter<LPUART1_IRQn>::dispatch();
}
