# SPDX-License-Identifier: GPL-3.0-or-later
# Host tests for the examples' models: plain C++ and Catch2, no board, no LVGL.
#   cmake -S tests -B tests/build -G Ninja -DCROSSPAD_CORE_DIR=<crosspad-core checkout>
#   cmake --build tests/build && ctest --test-dir tests/build --output-on-failure
cmake_minimum_required(VERSION 3.16)
project(crosspad-examples-tests CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CROSSPAD_CORE_DIR "" CACHE PATH "crosspad-core checkout; its include/ is on the path")
if(NOT EXISTS "${CROSSPAD_CORE_DIR}/include/crosspad")
    message(FATAL_ERROR "Set -DCROSSPAD_CORE_DIR to a crosspad-core checkout (got '${CROSSPAD_CORE_DIR}')")
endif()

include(FetchContent)
FetchContent_Declare(Catch2 GIT_REPOSITORY https://github.com/catchorg/Catch2.git GIT_TAG v3.5.0)
FetchContent_MakeAvailable(Catch2)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

file(GLOB TEST_SRCS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../examples/*/tests/*.cpp)
file(GLOB EXAMPLE_INCLUDES LIST_DIRECTORIES true ${CMAKE_CURRENT_SOURCE_DIR}/../examples/*/include)
if(NOT TEST_SRCS)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/empty.cpp "#include <catch2/catch_test_macros.hpp>\nTEST_CASE(\"no examples yet\") {}\n")
    set(TEST_SRCS ${CMAKE_CURRENT_BINARY_DIR}/empty.cpp)
endif()

add_executable(example_tests ${TEST_SRCS})
target_include_directories(example_tests PRIVATE ${EXAMPLE_INCLUDES} ${CROSSPAD_CORE_DIR}/include)
target_link_libraries(example_tests PRIVATE Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(example_tests)
