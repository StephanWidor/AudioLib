cmake_minimum_required(VERSION 3.25)
project(swAudioLib LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(${PROJECT_SOURCE_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
    set(IS_ROOT TRUE)
else()
    set(IS_ROOT FALSE)
endif()

option(SWAUDIOLIB_ENABLE_TESTS "Enable building Tests" ${IS_ROOT})

set(SOURCES
    src/sw/frequencyenvelope.hpp
    src/sw/notes.hpp
    src/sw/phases.hpp
    src/sw/processingbuffer.hpp
    src/sw/signals.hpp
    src/sw/spectrum.hpp
    src/sw/tuningnoteenvelope.hpp
    src/sw/variant.hpp
    src/sw/chrono/stopwatch.hpp
    src/sw/chrono/timer.hpp
    src/sw/containers/lockfreeswap.hpp
    src/sw/containers/spinlockedringbuffer.hpp
    src/sw/containers/utils.hpp
    src/sw/dft/spectrum.hpp
    src/sw/dft/transform.hpp
    src/sw/dft/utils.hpp
    src/sw/math/angles.hpp
    src/sw/math/concepts.hpp
    src/sw/math/constants.hpp
    src/sw/math/helpers.hpp
    src/sw/math/math.hpp
    src/sw/ranges/defines.hpp
    src/sw/ranges/utils.hpp
    )

add_library(${PROJECT_NAME} INTERFACE ${SOURCES})

find_package(TBB REQUIRED)

target_link_libraries(${PROJECT_NAME}
    INTERFACE
    TBB::tbb
    )

target_include_directories(${PROJECT_NAME} INTERFACE src)



if(SWAUDIOLIB_ENABLE_TESTS)
    add_subdirectory(tests)
endif()
