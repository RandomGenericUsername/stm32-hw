add_library(stm32_hw_wl5x_nucleo_wl55jc INTERFACE)

target_include_directories(stm32_hw_wl5x_nucleo_wl55jc INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(stm32_hw_wl5x_nucleo_wl55jc
    INTERFACE
        stm32_hw_wl5x
        stm32_hw_core
)
