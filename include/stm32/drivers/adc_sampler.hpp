#pragma once

#include <cstdint>
#include <tuple>

namespace stm32::drivers {

namespace detail {

// Binds a callable and its arguments for an interrupt callback.
//
// The tuple is static per (Adc, Fn, Args...). One sampler instance per ADC type
// is the intended model (the peripheral itself is a singleton), so a single
// registration per ADC is sufficient.
template <class Adc, auto Fn, class... Args>
class AdcCallback {
public:
    static void set(Args... args) {
        args_ = std::tuple<Args...>(args...);
    }

    static void invoke() {
        std::apply(Fn, args_);
    }

private:
    static inline std::tuple<Args...> args_{};
};

} // namespace detail

// Stateful ADC sampler: owns readiness/value and the completion callback.
//
// The application still owns the ISR symbol:
//
//   static AdcSampler<Adc1> g_adc;
//   extern "C" void ADC_IRQHandler(void) { g_adc.on_irq(); }
//
// Callback arguments are bound at registration time; the new sample is taken
// from the sampler (take()) inside the callback or later from the main loop.
template <class Adc>
class AdcSampler {
public:
    using SampleTime = typename Adc::SampleTime;

    AdcSampler() = default;
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
        if (callback_ != nullptr) {
            callback_();
        }
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

    template <auto Fn, class... Args>
    void on_complete(Args... args) {
        detail::AdcCallback<Adc, Fn, Args...>::set(args...);
        callback_ = &detail::AdcCallback<Adc, Fn, Args...>::invoke;
    }

private:
    using Callback = void (*)();

    Callback callback_ = nullptr;
    volatile bool ready_ = false;
    volatile std::uint16_t value_ = 0;
};

} // namespace stm32::drivers
