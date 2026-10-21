#pragma once

#include <cstdint>

#include <stm32h7xx.h>

#include "stm32/hw/adc.hpp"
#include "stm32/hw/clock/h7.hpp"
#include "stm32/hw/delay/systick.hpp"
#include "stm32/hw/nvic/h7.hpp"
#include "stm32/hw/peripheral.hpp"

namespace stm32::hw {

// STM32H7 ADC (ISR/IER/CR/CFGR/CFGR2/SMPR1-2/SQR1-4/DR).
//
// Default kernel clock: per_ck (HSI 64 MHz) / 4 = 16 MHz, with the analog boost
// configured for the 12.5..20 MHz band. Delay and NVIC are injected policies.
template <class Desc, class Delay = SysTickDelay, class Irq = H7Nvic>
class H7Adc final : public Adc<H7Adc<Desc, Delay, Irq>> {
public:
    enum class SampleTime : std::uint8_t {
        Cycles1_5 = 0,
        Cycles2_5 = 1,
        Cycles6_5 = 2,
        Cycles12_5 = 3,
        Cycles24_5 = 4,
        Cycles47_5 = 5,
        Cycles92_5 = 6,
        Cycles247_5 = 7,
    };

    H7Adc() = delete;
    H7Adc(const H7Adc&) = delete;
    H7Adc& operator=(const H7Adc&) = delete;

    static void init() {
        Delay::init();
        H7Clock::template enable<Desc::id>();
        H7Clock::select_adc_clock(H7Clock::AdcSource::ClkP);

        // per_ck / 4 (16 MHz at the reset 64 MHz HSI).
        Desc::common()->CCR = (Desc::common()->CCR & ~ADC_CCR_PRESC_Msk)
                            | ADC_CCR_PRESC_1;

        auto* adc = Desc::regs();

        if ((adc->CR & ADC_CR_ADVREGEN) == 0u) {
            adc->CR |= ADC_CR_ADVREGEN;
            Delay::wait_us(10); // regulator start-up (hardware-mandated)
        }

        if ((adc->CR & ADC_CR_ADEN) != 0u) {
            return;
        }

        // Analog boost for 12.5..20 MHz.
        adc->CR = (adc->CR & ~ADC_CR_BOOST) | ADC_CR_BOOST_1;

        // Offset calibration (single-ended).
        adc->CR |= ADC_CR_ADCAL;
        while ((adc->CR & ADC_CR_ADCAL) != 0u) {
        }

        adc->ISR = ADC_ISR_ADRDY;
        adc->CR |= ADC_CR_ADEN;
        while ((adc->ISR & ADC_ISR_ADRDY) == 0u) {
            if ((adc->CR & ADC_CR_ADEN) == 0u) {
                adc->CR |= ADC_CR_ADEN;
            }
        }
    }

    static void disable() {
        Irq::disable(irq());
        auto* adc = Desc::regs();
        adc->IER &= ~ADC_IER_EOCIE;
        if ((adc->CR & ADC_CR_ADEN) != 0u) {
            adc->CR |= ADC_CR_ADDIS;
            while ((adc->CR & ADC_CR_ADEN) != 0u) {
            }
        }
    }

    template <std::uint8_t Channel>
    static void configure(SampleTime sample_time) {
        static_assert(Channel < 20, "H7 ADC channel must be in 0..19");

        auto* adc = Desc::regs();

        // 12-bit, single, software trigger.
        adc->CFGR = (adc->CFGR
                     & ~(ADC_CFGR_RES_Msk | ADC_CFGR_CONT_Msk
                         | ADC_CFGR_EXTEN_Msk))
                  | (0x2u << ADC_CFGR_RES_Pos); // 12-bit

        constexpr std::uint32_t field = 3u;
        const std::uint32_t code = static_cast<std::uint32_t>(sample_time);
        if constexpr (Channel < 10) {
            constexpr std::uint32_t pos = ADC_SMPR1_SMP0_Pos + field * Channel;
            adc->SMPR1 = (adc->SMPR1 & ~(0x7u << pos)) | (code << pos);
        } else {
            constexpr std::uint32_t pos =
                ADC_SMPR2_SMP10_Pos + field * (Channel - 10u);
            adc->SMPR2 = (adc->SMPR2 & ~(0x7u << pos)) | (code << pos);
        }

        adc->SQR1 = (adc->SQR1 & ~(ADC_SQR1_L_Msk | ADC_SQR1_SQ1_Msk))
                  | (static_cast<std::uint32_t>(Channel)
                     << ADC_SQR1_SQ1_Pos);
    }

    static void start() {
        auto* adc = Desc::regs();
        adc->ISR = ADC_ISR_EOC; // clear stale EOC
        adc->IER |= ADC_IER_EOCIE;
        adc->CR |= ADC_CR_ADSTART;
    }

    static bool irq_pending() {
        return (Desc::regs()->ISR & ADC_ISR_EOC) != 0u;
    }

    static std::uint16_t read_result() {
        auto* adc = Desc::regs();
        const std::uint16_t value =
            static_cast<std::uint16_t>(adc->DR & 0x0FFFu);
        adc->ISR = ADC_ISR_EOC; // write 1 to clear
        return value;
    }

    static void enable_irq() {
        Irq::clear_pending(irq());
        Irq::enable(irq());
    }

    static void disable_irq() {
        Irq::disable(irq());
    }

    static void set_irq_priority(std::uint32_t priority) {
        Irq::set_priority(irq(), priority);
    }

    static constexpr IRQn_Type irq() {
        return (Desc::id == Peripheral::Adc3) ? ADC3_IRQn : ADC_IRQn;
    }

    template <std::uint8_t Channel>
    static std::uint16_t read() {
        static_assert(Channel < 20, "H7 ADC channel must be in 0..19");

        auto* adc = Desc::regs();
        adc->IER &= ~ADC_IER_EOCIE;
        adc->ISR = ADC_ISR_EOC;
        adc->CR |= ADC_CR_ADSTART;
        while ((adc->ISR & ADC_ISR_EOC) == 0u) {
        }
        const std::uint16_t value =
            static_cast<std::uint16_t>(adc->DR & 0x0FFFu);
        adc->ISR = ADC_ISR_EOC;
        return value;
    }
};

} // namespace stm32::hw
