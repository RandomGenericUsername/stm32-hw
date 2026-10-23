add_library(stm32_hw_core INTERFACE)

target_include_directories(stm32_hw_core INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(stm32_hw_core INTERFACE cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(stm32_hw_core INTERFACE
        -fno-exceptions
        -fno-rtti
    )
endif()
