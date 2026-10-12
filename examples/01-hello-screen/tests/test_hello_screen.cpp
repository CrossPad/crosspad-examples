// SPDX-License-Identifier: GPL-3.0-or-later
#include <catch2/catch_test_macros.hpp>
#include "example-hello-screen/HelloModel.hpp"

TEST_CASE("hello-screen: the label counts presses", "[hello-screen]")
{
    REQUIRE(example_hello::pressedLabel(0) == "Pressed 0 times");
    REQUIRE(example_hello::pressedLabel(1) == "Pressed 1 time");
    REQUIRE(example_hello::pressedLabel(12) == "Pressed 12 times");
}
