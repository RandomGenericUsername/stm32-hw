#pragma once

#include <cstdint>

#include <stm32wl55xx.h>

#include "stm32/hw/adc.hpp"
#include "stm32/hw/clock/wl55.hpp"
#include "stm32/hw/delay.hpp"
#include "stm32/hw/peripheral.hpp"

namespace stm32::hw {

// STM32WL55 ADC (L4-style: ISR/IER/CR/CFGR1/CFGR2/SMPR/CHSELR/DR).
//
// Default kernel clock: HSI16, asynchronous mode, no prescaler (16 MHz).
// The ADC has one SMPR with two shared sample-time fields (SMP1/SMP2) and a
// per-channel SMPSEL bit; this slice routes every channel to SMP1.
template <class Desc>
class WlAdc final : public Adc<WlAdc<Desc>> {
public:
    enum class SampleTime : std::uint8_t {
        Cycles1_5 = 0,
        Cycles3_5 = 1,
        Cycles7_5 = 2,
        Cycles12_5 = 3,
        Cycles19_5 = 4,
        Cycles39_5 = 5,
        Cycles79_5 = 6,
        Cycles160_5 = 7,
    };

    WlAdc() = delete;
    WlAdc(const WlAdc&) = delete;
    WlAdc& operator=(const WlAdc&) = delete;

    static void init() {
        WlClock::template enable<Desc::id>();
        WlClock::select_adc_clock(WlClock::AdcSource::Hsi16);

        auto* adc = Desc::regs();

        // Asynchronous kernel clock, no prescaler (HSI16 = 16 MHz).
        adc->CFGR2 &= ~ADC_CFGR2_CKMODE;
        Desc::common()->CCR &= ~ADC_CCR_PRESC;

        if ((adc->CR & ADC_CR_ADVREGEN) == 0u) {
            adc->CR |= ADC_CR_ADVREGEN;
            delay_us(20); // regulator start-up
        }

        if ((adc->CR & ADC_CR_ADEN) != 0u) {
            return;
        }

        adc->CR |= ADC_CR_ADCAL;
        while ((adc->CR & ADC_CR_ADCAL) != 0u) {
        }

        adc->ISR = ADC_ISR_ADRDY;
        adc->CR |= ADC_CR_ADEN;
        while ((adc->ISR & ADC_ISR_ADRDY) == 0u) {
            // After calibration, ADEN may be reset if set too soon.
            if ((adc->CR & ADC_CR_ADEN) == 0u) {
                adc->CR |= ADC_CR_ADEN;
            }
        }
    }

    static void disable() {
        auto* adc = Desc::regs();
        if ((adc->CR & ADC_CR_ADEN) != 0u) {
            adc->CR |= ADC_CR_ADDIS;
            while ((adc->CR & ADC_CR_ADEN) != 0u) {
            }
        }
    }

    template <std::uint8_t Channel>
    static void configure(SampleTime sample_time) {
        static_assert(Channel < 18, "WL55 ADC channel must be in 0..17");

        auto* adc = Desc::regs();

        // 12-bit, single, software trigger, right-aligned, bitmap CHSELR.
        adc->CFGR1 &= ~(ADC_CFGR1_RES_Msk | ADC_CFGR1_CONT | ADC_CFGR1_EXTEN
                        | ADC_CFGR1_ALIGN | ADC_CFGR1_CHSELRMOD);

        const std::uint32_t code =
            static_cast<std::uint32_t>(sample_time);
        adc->SMPR = (adc->SMPR & ~ADC_SMPR_SMP1_Msk)
                  | (code << ADC_SMPR_SMP1_Pos);

        // Use SMP1 for this channel.
        adc->SMPR &= ~(ADC_SMPR_SMPSEL0_Msk << Channel);

        // Channel selection is a bitmask; wait for CCRDY after programming it.
        adc->ISR = ADC_ISR_CCRDY;
        adc->CHSELR = (1u << Channel);
        while ((adc->ISR & ADC_ISR_CCRDY) == 0u) {
        }
    }

    template <std::uint8_t Channel>
    static std::uint16_t read() {
        static_assert(Channel < 18, "WL55 ADC channel must be in 0..17");

        auto* adc = Desc::regs();
        adc->ISR = ADC_ISR_EOC;
        adc->CR |= ADC_CR_ADSTART;
        while ((adc->ISR & ADC_ISR_EOC) == 0u) {
        }
        return static_cast<std::uint16_t>(adc->DR & 0x0FFFu);
    }
};

} // namespace stm32::hw
