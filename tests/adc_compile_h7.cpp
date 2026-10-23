// Compile check for the H7 ADC driver path (NUCLEO-H755ZI-Q reference).
// ADC1/2 share ADC_IRQn; ADC3 has ADC3_IRQn.
// Compile with -mcpu=cortex-m7 -DSTM32H755xx -DCORE_CM7 and H7 CMSIS includes.

#include <cstdint>

#include "stm32/drivers/h7.hpp"

struct Adc1Handler {
    void on_complete(std::uint16_t value) {
        (void)value;
    }
};

struct Adc3Handler {
    void on_complete(std::uint16_t value) {
        (void)value;
    }
};

using Adc1 = stm32::drivers::h7::Adc1<Adc1Handler>;
using Adc3 = stm32::drivers::h7::Adc3<Adc3Handler>;

extern "C" std::uint16_t adc_h7_check() {
    Adc1::init();
    Adc1::set_irq_priority(15);
    Adc1::start<0>(Adc1::SampleTime::Cycles12_5);

    Adc3::init();
    Adc3::set_irq_priority(15);
    Adc3::start<0>(Adc3::SampleTime::Cycles12_5);

    const std::uint16_t a = Adc1::ready() ? Adc1::take() : 0u;
    const std::uint16_t b = Adc3::ready() ? Adc3::take() : 0u;
    return static_cast<std::uint16_t>(a ^ b);
}
