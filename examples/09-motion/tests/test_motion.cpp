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
    REQUIRE(worthSending(64, 62));   // a step down of two counts, too
    REQUIRE_FALSE(worthSending(64, 64));
}

TEST_CASE("motion: the interior of the range rounds to the nearest value", "[motion]")
{
    REQUIRE(tiltToCc(-22.5f) == 32);   // 31.75
    REQUIRE(tiltToCc(22.5f) == 95);    // 95.25
    REQUIRE(tiltToCc(-44.0f) == 1);    // 1.41
    REQUIRE(tiltToCc(44.0f) == 126);   // 125.59
}

TEST_CASE("motion: a sweep never goes backwards and covers 0..127", "[motion]")
{
    int prev = -1;
    for (float deg = -50.0f; deg <= 50.0f; deg += 0.25f) {
        const int cc = tiltToCc(deg);
        REQUIRE(cc >= prev);
        prev = cc;
    }
    REQUIRE(prev == 127);
}

TEST_CASE("motion: a slow sweep still reaches both ends", "[motion]")
{
    // Steps of one never pass the "moved by 2" rule, but the ends must be sent.
    REQUIRE(worthSending(126, 127));
    REQUIRE(worthSending(1, 0));
    REQUIRE_FALSE(worthSending(127, 127));
    REQUIRE_FALSE(worthSending(0, 0));
    REQUIRE_FALSE(worthSending(125, 126));   // only the ends are special
}
