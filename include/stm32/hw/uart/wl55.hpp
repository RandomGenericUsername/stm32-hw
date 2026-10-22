#pragma once

#include <cstdint>

#include <stm32wl55xx.h>

#include "stm32/hw/clock/wl55.hpp"
#include "stm32/hw/nvic/wl55.hpp"
#include "stm32/hw/uart.hpp"
#include "stm32/hw/uart/config.hpp"

namespace stm32::hw {

// STM32WL55 LPUART (USART IP in low-power mode).
//
// The application supplies the kernel-clock facts. BRR is the LPUART divider
// (RM0453 §30.5.4): baud = 256 × f_ck / BRR, so BRR = 256 × f_ck / baud.
// The divider is 16-bit and valid from 0x300 to 0xFFFF.
template <class Desc, class Irq = WlNvic>
class WlLpuart final
    : public Uart<WlLpuart<Desc, Irq>, typename Desc::UartClockTraits> {
public:
    using Clock = WlClock<typename Desc::ClockTraits>;
    using Config = UartConfig<typename Desc::UartClockTraits>;

    static constexpr std::uint32_t min_brr = 0x300u;
    static constexpr std::uint32_t max_brr = 0xFFFFu;

    WlLpuart() = delete;
    WlLpuart(const WlLpuart&) = delete;
    WlLpuart& operator=(const WlLpuart&) = delete;

    // Rounded LPUART divider for a kernel frequency and baud rate.
    static constexpr std::uint32_t brr_for(std::uint32_t kernel_hz,
                                           std::uint32_t baud) {
        if (baud == 0u) {
            return 0u;
        }
        return static_cast<std::uint32_t>(
            (256ull * static_cast<unsigned long long>(kernel_hz)
             + baud / 2u) / baud);
    }

    // 8N1, transmitter only. Returns false if the baud rate cannot be
    // represented at the supplied kernel frequency.
    static bool init(const Config& config) {
        const std::uint32_t brr = brr_for(config.kernel_hz, config.baud);
        if (brr < min_brr || brr > max_brr) {
            return false;
        }

        Clock::template enable<Desc::id>();
        Clock::select_lpuart1_clock(
            static_cast<std::uint32_t>(config.source));

        auto* uart = Desc::regs();
        uart->CR1 = 0u; // UE = 0 while configuring
        uart->CR2 = 0u;
        uart->CR3 = 0u;
        uart->BRR = brr;
        uart->CR1 = USART_CR1_TE | USART_CR1_UE;
        return true;
    }

    static void disable() {
        Irq::disable(Desc::irq());
        auto* uart = Desc::regs();
        uart->CR1 &= ~(USART_CR1_TXEIE_TXFNFIE | USART_CR1_TE | USART_CR1_UE);
    }

    static void write_byte(std::uint8_t value) {
        Desc::regs()->TDR = value;
    }

    static bool tx_ready() {
        return (Desc::regs()->ISR & USART_ISR_TXE_TXFNF) != 0u;
    }

    static bool tx_complete() {
        return (Desc::regs()->ISR & USART_ISR_TC) != 0u;
    }

    static void enable_tx_irq() {
        Desc::regs()->CR1 |= USART_CR1_TXEIE_TXFNFIE;
        Irq::enable(Desc::irq());
    }

    static void disable_tx_irq() {
        Desc::regs()->CR1 &= ~USART_CR1_TXEIE_TXFNFIE;
    }

    static bool tx_irq_pending() {
        return tx_ready(); // TXE is the level that drives the interrupt
    }

    static void set_irq_priority(std::uint32_t priority) {
        Irq::set_priority(Desc::irq(), priority);
    }

    static constexpr auto irq() {
        return Desc::irq();
    }
};

} // namespace stm32::hw
