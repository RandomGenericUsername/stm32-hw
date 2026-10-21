#pragma once

#include <cstdint>

namespace stm32::drivers {

// Stateful ADC sampler. The callback is part of the sampler type and is
// injected at construction; its state lives in the callback object.
//
// Callback contract:
//   void operator()(std::uint16_t value) const
//
// Usage:
//   struct OnAdc { State& state;
//                  void operator()(std::uint16_t v) const { ... } };
//   using Sampler = AdcSampler<Adc1, OnAdc>;
//   static Sampler g_adc(OnAdc{g_state});
//
// The application still owns the ISR symbol:
//   extern "C" void ADC_IRQHandler(void) { g_adc.on_irq(); }
template <class Adc, class Callback>
class AdcSampler {
public:
    using SampleTime = typename Adc::SampleTime;

    constexpr explicit AdcSampler(Callback callback)
        : callback_(callback) {}

    AdcSampler(const AdcSampler&) = delete;
    AdcSampler& operator=(const AdcSampler&) = delete;

    void init() {
        Adc::init();
        ready_ = false;
        value_ = 0;
    }

    template <std::uint8_t Channel>
    void start(SampleTime sample_time) {
        ready_ = false;
        Adc::template configure<Channel>(sample_time);
        Adc::start();
        Adc::enable_irq();
    }

    bool on_irq() {
        if (!Adc::irq_pending()) {
            return false;
        }
        value_ = Adc::read_result();
        ready_ = true;
        callback_(value_);
        return true;
    }

    bool ready() const {
        return ready_;
    }

    std::uint16_t take() {
        ready_ = false;
        return value_;
    }

    void stop() {
        Adc::disable_irq();
        Adc::disable();
    }

    void set_irq_priority(std::uint32_t priority) {
        Adc::set_irq_priority(priority);
    }

private:
    Callback callback_;
    volatile bool ready_ = false;
    volatile std::uint16_t value_ = 0;
};

} // namespace stm32::drivers
