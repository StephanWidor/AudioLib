cmake_minimum_required(VERSION 3.25)
project(swAudioLib-tests LANGUAGES CXX)

add_executable(${PROJECT_NAME}
    chrono.cpp
    notes.cpp
    sinewavedetection.cpp
    containers/lockfreeswap.cpp
    containers/spinlockedringbuffer.cpp
    dft/dft.cpp
    dft/spectrum.cpp
    math/angles.cpp
    )

find_package(GTest REQUIRED)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    swAudioLib
    GTest::GTest
    GTest::Main
    )

enable_testing()
include(GoogleTest)

gtest_discover_tests(${PROJECT_NAME})
