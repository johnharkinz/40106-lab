cmake_minimum_required(VERSION 3.20)
project(40106Lab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Portable engine: no platform, UI or file dependencies.
add_library(engine engine/SchmittInverter.cpp)
target_include_directories(engine PUBLIC engine)
if(NOT MSVC)
    # The FM-1 has a single-precision FPU: flag accidental double maths.
    target_compile_options(engine PRIVATE -Wdouble-promotion)
endif()

add_executable(render_wav desktop/render_wav.cpp desktop/WavWriter.cpp)
target_link_libraries(render_wav PRIVATE engine)

enable_testing()
add_executable(oscillator_tests tests/oscillator_tests.cpp)
target_link_libraries(oscillator_tests PRIVATE engine)
add_test(NAME oscillator_tests COMMAND oscillator_tests)
