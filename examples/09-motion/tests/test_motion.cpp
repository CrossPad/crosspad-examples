// SPDX-License-Identifier: GPL-3.0-or-later
#include <catch2/catch_test_macros.hpp>
#include "example-motion/MotionModel.hpp"

using namespace example_motion;

TEST_CASE("motion: tilt spans the CC range and holds past 45 degrees", "[motion]")
{
    REQUIRE(tiltToCc(-90.0f) == 0);
    REQUIRE(tiltToCc(-45.0f) == 0);
    REQUIRE(tiltToCc(0.0f) == 64);
    REQUIRE(tiltToCc(45.0f) == 127);
    REQUIRE(tiltToCc(80.0f) == 127);
}

TEST_CASE("motion: jitter of one is not sent", "[motion]")
{
    REQUIRE(worthSending(-1, 64));
    REQUIRE_FALSE(worthSending(64, 65));
    REQUIRE_FALSE(worthSending(64, 63));
    REQUIRE(worthSending(64, 66));
}
