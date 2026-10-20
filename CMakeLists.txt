cmake_minimum_required(VERSION 3.20)

project(stm32_hw LANGUAGES CXX)

add_library(stm32_hw INTERFACE)

target_include_directories(stm32_hw INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(stm32_hw INTERFACE cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(stm32_hw INTERFACE
        -fno-exceptions
        -fno-rtti
    )
endif()
