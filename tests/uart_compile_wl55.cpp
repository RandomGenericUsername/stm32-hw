// Compile check for the WL55 LPUART driver path (NUCLEO-WL55JC reference).
// Compile with -mcpu=cortex-m4 -DSTM32WL55xx and the WL55 CMSIS includes.

#include <cstdint>

#include "stm32/board/nucleo_wl55jc.hpp"
#include "stm32/drivers/wl55.hpp"

using Uart1 = stm32::drivers::wl55::Lpuart1;
using HwUart = stm32::hw::WlLpuart<stm32::device::wl55::Lpuart1>;
using UartConfig = stm32::hw::UartConfig<stm32::device::wl55::UartClockTraits>;

// LPUART divider: BRR = 256 x f_ck / baud, rounded.
static_assert(HwUart::brr_for(16000000u, 115200u) == 35556u,
              "16 MHz / 115200 must be BRR 35556");
static_assert(HwUart::brr_for(32000000u, 115200u) == 71111u,
              "32 MHz / 115200 must be BRR 71111");
static_assert(HwUart::brr_for(32768u, 9600u) == 874u,
              "LSE / 9600 must be BRR 874");
static_assert(HwUart::brr_for(16000000u, 0u) == 0u,
              "zero baud must not divide by zero");

constexpr auto config = UartConfig::from_system(
    stm32::board::nucleo_wl55jc::hsi16_16mhz,
    115200u,
    stm32::device::wl55::UartClockTraits::Source::Pclk1);
static_assert(config.kernel_hz == 16000000u);
static_assert(config.receive);

extern "C" std::uint32_t uart_wl55_check() {
    if (!Uart1::init(config)) {
        return 1u;
    }
    Uart1::set_irq_priority(6u);
    Uart1::write("stm32-hw uart check\n");
    Uart1::write_byte(static_cast<std::uint8_t>('!'));
    Uart1::flush();

    std::uint8_t received = 0u;
    if (Uart1::read(received)) {
        Uart1::write_byte(received);
    }
    return static_cast<std::uint32_t>(Uart1::pending() + Uart1::rx_pending());
}
