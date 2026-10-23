#pragma once

#include <concepts>
#include <cstdint>

#include "stm32/core/drivers/adc_irq.hpp"

namespace stm32::drivers {

// Default handler: no application event.
struct AdcNullHandler {
    void on_complete(std::uint16_t) {}
};

// Handler contract for AdcDriver. The driver owns one Handler instance and
// calls on_complete(value) from the ADC interrupt.
template <class H>
concept AdcHandler =
    std::default_initializable<H> &&
    requires(H& handler, std::uint16_t value) {
        { handler.on_complete(value) } -> std::same_as<void>;
    };

// Type-level ADC driver.
//
// HwAdc is the stateless hardware facade (F4Adc / H7Adc / WlAdc). Handler is
// a compile-time policy whose on_complete(value) runs in the ADC interrupt.
// The driver owns the handler object by value; there is no callback pointer
// and no context lifetime problem. The ISR symbol is emitted by the compiled
// family source (<family>/src/irq/adc.cpp), which calls the router.
template <class HwAdc, AdcHandler Handler = AdcNullHandler>
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

    // The handler is owned by the driver. Its lifetime is the driver's.
    static Handler& handler() {
        return handler_;
    }

private:
    static bool on_irq() {
        if (!HwAdc::irq_pending()) {
            return false;
        }
        value_ = HwAdc::read_result();
        ready_ = true;
        busy_ = false;
        handler_.on_complete(value_);
        return true;
    }

    static inline volatile bool busy_ = false;
    static inline volatile bool ready_ = false;
    static inline volatile std::uint16_t value_ = 0;
    static inline Handler handler_{};
};

} // namespace stm32::drivers
