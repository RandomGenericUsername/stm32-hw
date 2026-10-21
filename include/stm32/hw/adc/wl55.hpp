#pragma once

#include <cstdint>

#include <stm32wl55xx.h>

#include "stm32/device/wl55/clock.hpp"
#include "stm32/hw/adc.hpp"
#include "stm32/hw/adc/clock.hpp"
#include "stm32/hw/clock/wl55.hpp"
#include "stm32/hw/delay/systick.hpp"
#include "stm32/hw/nvic/wl55.hpp"
#include "stm32/hw/peripheral.hpp"

namespace stm32::hw {

// Default WL55 ADC clock policy: HSI16 asynchronous, PRESC /1 = 16 MHz.
template <class AdcTraits>
struct WlAdcClockDefault {
    static constexpr AdcClockConfig<AdcTraits> config = {
        AdcTraits::Source::Hsi16,
        AdcTraits::Mode::Asynchronous,
        AdcTraits::Prescaler::Div1,
    };
};

// STM32WL55 ADC (L4-style: ISR/IER/CR/CFGR1/CFGR2/SMPR/CHSELR/DR).
//
// Default kernel clock: HSI16, asynchronous mode, no prescaler (16 MHz).
// Delay, NVIC and ADC clock are injected policies.
template <class Desc, class Delay = SysTickDelay, class Irq = WlNvic,
          class ClockPolicy =
              WlAdcClockDefault<typename Desc::AdcClockTraits>>
class WlAdc final : public Adc<WlAdc<Desc, Delay, Irq, ClockPolicy>> {
public:
    using Clock = WlClock<typename Desc::ClockTraits>;
    using AdcTraits = typename Desc::AdcClockTraits;
    static constexpr auto adc_clock = ClockPolicy::config;

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
        Delay::init();
        Clock::template enable<Desc::id>();

        // The ADC owns the source choice; RCC only writes the ADCSEL mux.
        if (adc_clock.source == AdcTraits::Source::Hsi16) {
            Clock::enable_hsi16();
        }
        Clock::select_adc_clock(
            static_cast<std::uint32_t>(adc_clock.source));

        auto* adc = Desc::regs();

        // ADC-local clock mode and asynchronous prescaler.
        adc->CFGR2 = (adc->CFGR2 & ~ADC_CFGR2_CKMODE_Msk)
                   | (static_cast<std::uint32_t>(adc_clock.mode)
                      << ADC_CFGR2_CKMODE_Pos);
        Desc::common()->CCR =
            (Desc::common()->CCR & ~ADC_CCR_PRESC_Msk)
            | (static_cast<std::uint32_t>(adc_clock.prescaler)
               << ADC_CCR_PRESC_Pos);

        if ((adc->CR & ADC_CR_ADVREGEN) == 0u) {
            adc->CR |= ADC_CR_ADVREGEN;
            Delay::wait_us(20); // regulator start-up (hardware-mandated)
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
        static_assert(Channel < 18, "WL55 ADC channel must be in 0..17");

        auto* adc = Desc::regs();

        // 12-bit, single, software trigger, right-aligned, bitmap CHSELR.
        adc->CFGR1 &= ~(ADC_CFGR1_RES_Msk | ADC_CFGR1_CONT | ADC_CFGR1_EXTEN
                        | ADC_CFGR1_ALIGN | ADC_CFGR1_CHSELRMOD);

        const std::uint32_t code = static_cast<std::uint32_t>(sample_time);
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

    // WL55-specific internal path: VREFINT is ADC VIN[13].
    static void enable_vrefint() {
        Desc::common()->CCR |= ADC_CCR_VREFEN;
    }

    // Factory VREFINT calibration count from system memory (read-only).
    static std::uint16_t vrefint_cal() {
        return *reinterpret_cast<const volatile std::uint16_t*>(
            Desc::vrefint_cal_address);
    }

    // VREF+ characterized during the VREFINT calibration (millivolts).
    static constexpr std::uint32_t vrefplus_charac_mv = Desc::vrefplus_charac_mv;

    static constexpr IRQn_Type irq() {
        return ADC_IRQn;
    }

    template <std::uint8_t Channel>
    static std::uint16_t read() {
        static_assert(Channel < 18, "WL55 ADC channel must be in 0..17");

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
