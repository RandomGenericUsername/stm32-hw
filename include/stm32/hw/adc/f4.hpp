#pragma once

#include <cstdint>

#include <stm32f4xx.h>

#include "stm32/hw/adc.hpp"
#include "stm32/hw/clock/f4.hpp"
#include "stm32/hw/delay.hpp"
#include "stm32/hw/peripheral.hpp"

namespace stm32::hw {

// STM32F4 ADC (legacy ADC IP: SR/CR1/CR2/SMPR1-2/SQR1-3/DR).
//
// Clocking: the ADC is fed from PCLK2 through ADC_CCR.ADCPRE. The default
// /2 is safe for the reset HSI/PCLK2 configuration; a configurable divider is
// a later slice.
template <class Desc>
class F4Adc final : public Adc<F4Adc<Desc>> {
public:
    enum class SampleTime : std::uint8_t {
        Cycles3 = 0,
        Cycles15 = 1,
        Cycles28 = 2,
        Cycles56 = 3,
        Cycles84 = 4,
        Cycles112 = 5,
        Cycles144 = 6,
        Cycles480 = 7,
    };

    F4Adc() = delete;
    F4Adc(const F4Adc&) = delete;
    F4Adc& operator=(const F4Adc&) = delete;

    static void init() {
        F4Clock::template enable<Desc::id>();

        // PCLK2 / 2 (ADCPRE = 00). At reset PCLK2 is 16 MHz -> 8 MHz ADC.
        Desc::common()->CCR &= ~ADC_CCR_ADCPRE;

        auto* adc = Desc::regs();
        if ((adc->CR2 & ADC_CR2_ADON) == 0u) {
            adc->CR2 |= ADC_CR2_ADON;
            delay_us(3); // tSTAB
        }
    }

    static void disable() {
        Desc::regs()->CR2 &= ~ADC_CR2_ADON;
    }

    template <std::uint8_t Channel>
    static void configure(SampleTime sample_time) {
        static_assert(Channel < 19, "F4 ADC channel must be in 0..18");

        auto* adc = Desc::regs();

        // 12-bit, single conversion, no scan, right-aligned, software trigger.
        adc->CR1 = (adc->CR1 & ~(ADC_CR1_RES_Msk | ADC_CR1_SCAN_Msk));
        adc->CR2 &= ~(ADC_CR2_CONT | ADC_CR2_ALIGN | ADC_CR2_EXTEN);

        constexpr std::uint32_t field = 3u;
        const std::uint32_t code =
            static_cast<std::uint32_t>(sample_time);
        if constexpr (Channel < 10) {
            constexpr std::uint32_t pos = ADC_SMPR2_SMP0_Pos + field * Channel;
            adc->SMPR2 = (adc->SMPR2 & ~(0x7u << pos)) | (code << pos);
        } else {
            constexpr std::uint32_t pos =
                ADC_SMPR1_SMP10_Pos + field * (Channel - 10u);
            adc->SMPR1 = (adc->SMPR1 & ~(0x7u << pos)) | (code << pos);
        }

        // Sequence length 1, first conversion = Channel.
        adc->SQR1 &= ~ADC_SQR1_L_Msk;
        adc->SQR3 = (adc->SQR3 & ~ADC_SQR3_SQ1_Msk)
                  | (static_cast<std::uint32_t>(Channel)
                     << ADC_SQR3_SQ1_Pos);
    }

    template <std::uint8_t Channel>
    static std::uint16_t read() {
        static_assert(Channel < 19, "F4 ADC channel must be in 0..18");

        auto* adc = Desc::regs();
        adc->CR2 |= ADC_CR2_SWSTART;
        while ((adc->SR & ADC_SR_EOC) == 0u) {
        }
        return static_cast<std::uint16_t>(adc->DR & 0x0FFFu);
    }
};

} // namespace stm32::hw
