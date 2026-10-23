#pragma once

#include <cstdint>

namespace stm32::hw {

// Edge(s) that can trigger an external interrupt line.
enum class Edge : std::uint8_t {
    Rising = 1,
    Falling = 2,
    Both = 3,
};

// Abstract external-interrupt facade.
//
// This is a type-level singleton: no Exti object is constructed. The concrete
// implementation (WlExti, F4Exti, ...) supplies the static methods.
template <class Impl>
class Exti {
public:
    Exti() = delete;
    Exti(const Exti&) = delete;
    Exti& operator=(const Exti&) = delete;

    template <class PortDesc, std::uint8_t Line>
    static void configure(Edge edge) {
        Impl::template configure<PortDesc, Line>(edge);
    }

    template <std::uint8_t Line>
    static void enable() {
        Impl::template enable<Line>();
    }

    template <std::uint8_t Line>
    static void disable() {
        Impl::template disable<Line>();
    }

    template <std::uint8_t Line>
    static void clear_pending() {
        Impl::template clear_pending<Line>();
    }

    template <std::uint8_t Line>
    static bool is_pending() {
        return Impl::template is_pending<Line>();
    }
};

} // namespace stm32::hw
