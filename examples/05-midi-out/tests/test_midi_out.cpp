// SPDX-License-Identifier: GPL-3.0-or-later
#include <catch2/catch_test_macros.hpp>
#include "example-midi-out/MidiModel.hpp"

using namespace example_midi;

TEST_CASE("midi-out: pads map to the GM drum notes from C2", "[midi-out]")
{
    REQUIRE(noteFor(0) == 36);
    REQUIRE(noteFor(15) == 51);
}

TEST_CASE("midi-out: pan covers the whole bend range, centre is zero", "[midi-out]")
{
    REQUIRE(panToBend(0) == 0);
    REQUIRE(panToBend(127) == 8191);
    REQUIRE(panToBend(-127) == -8192);
    REQUIRE(panToBend(-128) == -8192);
}

TEST_CASE("midi-out: channel steps wrap round 16", "[midi-out]")
{
    REQUIRE(nextChannel(15, +1) == 0);
    REQUIRE(nextChannel(0, -1) == 15);
}
