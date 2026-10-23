#pragma once

#include <cstdint>

namespace stm32::hw {

enum class Mode : std::uint8_t {
    Input = 0,
    Output = 1,
    Alternate = 2,
    Analog = 3,
};

enum class OutputType : std::uint8_t {
    PushPull = 0,
    OpenDrain = 1,
};

enum class Speed : std::uint8_t {
    Low = 0,
    Medium = 1,
    High = 2,
    VeryHigh = 3,
};

enum class Pull : std::uint8_t {
    None = 0,
    Up = 1,
    Down = 2,
};

// Passive configuration aggregate. No invariants, no behavior.
struct GpioConfig {
    Mode mode = Mode::Input;
    OutputType output = OutputType::PushPull;
    Speed speed = Speed::Low;
    Pull pull = Pull::None;
    std::uint8_t alternate_function = 0;
};

} // namespace stm32::hw
