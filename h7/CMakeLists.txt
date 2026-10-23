add_library(stm32_hw_h7 INTERFACE)

target_include_directories(stm32_hw_h7 INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_sources(stm32_hw_h7 INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/irq/adc.cpp
)

target_link_libraries(stm32_hw_h7 INTERFACE stm32_hw_core)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(stm32_hw_h7 INTERFACE
        -fno-exceptions
        -fno-rtti
    )
endif()
