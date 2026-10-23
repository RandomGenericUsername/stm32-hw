#pragma once

#include <cstdint>

#include "stm32/device/wl55/adc.hpp"
#include "stm32/device/wl55/uart.hpp"
#include "stm32/drivers/adc.hpp"
#include "stm32/drivers/uart.hpp"
#include "stm32/hw/adc/wl55.hpp"
#include "stm32/hw/uart/wl55.hpp"

namespace stm32::drivers::wl55 {

template <class Handler = AdcNullHandler>
class Adc1 final
    : public AdcDriver<hw::WlAdc<device::wl55::Adc1>, Handler> {
public:
    using HwAdc = hw::WlAdc<device::wl55::Adc1>;

    static void enable_vrefint() {
        HwAdc::enable_vrefint();
    }

    // Factory VREFINT calibration count (system memory).
    static std::uint16_t vrefint_cal() {
        return HwAdc::vrefint_cal();
    }

    // RM0453 §18.9, "Calculating the actual VREF+ voltage":
    //   VREF+ = VREF+_Charac × VREFINT_CAL / VREFINT_DATA
    // Returns the calibrated analog reference/supply (VREF+), in millivolts,
    // from one raw VREFINT sample. Not the ~1.2 V bandgap voltage itself.
    // The two-argument form is pure so it can be checked at compile time.
    static constexpr std::uint32_t vrefplus_millivolts(std::uint16_t cal,
                                                       std::uint16_t raw) {
        if (raw == 0u) {
            return 0u;
        }
        return (HwAdc::vrefplus_charac_mv * static_cast<std::uint32_t>(cal)
                + raw / 2u) / raw;
    }

    static std::uint32_t vrefplus_millivolts(std::uint16_t vrefint_raw) {
        return vrefplus_millivolts(HwAdc::vrefint_cal(), vrefint_raw);
    }
};

template <class Handler = UartNullHandler>
class Lpuart1 final
    : public UartDriver<
          hw::WlLpuart<device::wl55::Lpuart1>, 128, 128, Handler> {};

} // namespace stm32::drivers::wl55
