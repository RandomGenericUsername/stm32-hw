#pragma once

#include <cstdint>
#include <tuple>

namespace stm32::drivers {

// Stateful ADC sampler: the callback is part of the type and is injected at
// construction. No static callback state, no registration step.
//
// Callback signature:
//   void(std::uint16_t value, Args...)
//
// Usage:
//   void on_adc(std::uint16_t value, Context* ctx) { ... }
//   using Sampler = AdcSampler<Adc1, decltype(&on_adc), Context*>;
//   static Sampler g_adc(&on_adc, &ctx);
//
// The application still owns the ISR symbol:
//   extern "C" void ADC_IRQHandler(void) { g_adc.on_irq(); }
template <class Adc, class Callback, class... Args>
class AdcSampler {
public:
    using SampleTime = typename Adc::SampleTime;

    constexpr AdcSampler(Callback callback, Args... args)
        : callback_(callback), args_(args...) {}

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
        invoke();
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
    void invoke() {
        std::apply(
            [this](auto&... args) { callback_(value_, args...); },
            args_);
    }

    Callback callback_;
    std::tuple<Args...> args_;
    volatile bool ready_ = false;
    volatile std::uint16_t value_ = 0;
};

} // namespace stm32::drivers
