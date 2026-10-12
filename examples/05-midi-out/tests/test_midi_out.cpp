// SPDX-License-Identifier: GPL-3.0-or-later
#include <catch2/catch_test_macros.hpp>
#include "example-midi-out/MidiModel.hpp"

using namespace example_midi;

TEST_CASE("midi-out: pads map to the GM drum notes from C2", "[midi-out]")
{
    REQUIRE(noteFor(0) == 36);
    REQUIRE(noteFor(15) == 51);
}

TEST_CASE("midi-out: pan -64..63 covers the whole bend range, centre is zero", "[midi-out]")
{
    REQUIRE(kPanMin == -64);
    REQUIRE(kPanMax == 63);
    REQUIRE(panToBend(0) == 0);
    REQUIRE(panToBend(kPanMin) == -8192);
    REQUIRE(panToBend(kPanMax) == 8191);
    REQUIRE(panToBend(-32) == -4096);
    REQUIRE(panToBend(1) > 0);
    REQUIRE(panToBend(-1) < 0);
}

TEST_CASE("midi-out: pan outside the range is clamped", "[midi-out]")
{
    REQUIRE(panToBend(-65) == -8192);
    REQUIRE(panToBend(-128) == -8192);
    REQUIRE(panToBend(64) == 8191);
    REQUIRE(panToBend(127) == 8191);
}

TEST_CASE("midi-out: pan is monotonic over the range", "[midi-out]")
{
    int prev = panToBend(kPanMin);
    for (int p = kPanMin + 1; p <= kPanMax; ++p) {
        const int b = panToBend(static_cast<int8_t>(p));
        REQUIRE(b > prev);
        prev = b;
    }
}

TEST_CASE("midi-out: channel steps wrap round 16", "[midi-out]")
{
    REQUIRE(nextChannel(15, +1) == 0);
    REQUIRE(nextChannel(0, -1) == 15);
}
