#pragma once

#include <cstdint>

#include "stm32/drivers/adc_irq.hpp"

namespace stm32::drivers {

// Type-level ADC driver.
//
// HwAdc is the stateless hardware facade (F4Adc / H7Adc / WlAdc). This class
// owns the conversion state, the completion callback and the interrupt
// bookkeeping. The ISR symbol is emitted by the compiled driver source
// (src/drivers/adc_irq_<family>.cpp), which calls the router.
template <class HwAdc>
class AdcDriver {
public:
    using SampleTime = typename HwAdc::SampleTime;

    static void init() {
        HwAdc::init();
        busy_ = false;
        ready_ = false;
        value_ = 0;
        AdcIrqRouter<HwAdc::irq()>::attach(&on_irq);
    }

    static void stop() {
        HwAdc::disable_irq();
        HwAdc::disable();
        busy_ = false;
    }

    template <std::uint8_t Channel>
    static void start(SampleTime sample_time) {
        if (busy_) {
            return;
        }
        ready_ = false;
        busy_ = true;
        HwAdc::template configure<Channel>(sample_time);
        HwAdc::start();
        HwAdc::enable_irq();
    }

    static bool busy() {
        return busy_;
    }

    static bool ready() {
        return ready_;
    }

    static std::uint16_t take() {
        ready_ = false;
        return value_;
    }

    static void set_irq_priority(std::uint32_t priority) {
        HwAdc::set_irq_priority(priority);
    }

    // Optional semantic hook; runs in the ADC interrupt.
    static void on_complete(void (*callback)(std::uint16_t)) {
        callback_ = callback;
    }

private:
    static bool on_irq() {
        if (!HwAdc::irq_pending()) {
            return false;
        }
        value_ = HwAdc::read_result();
        ready_ = true;
        busy_ = false;
        if (callback_ != nullptr) {
            callback_(value_);
        }
        return true;
    }

    static inline volatile bool busy_ = false;
    static inline volatile bool ready_ = false;
    static inline volatile std::uint16_t value_ = 0;
    static inline void (*callback_)(std::uint16_t) = nullptr;
};

} // namespace stm32::drivers
