// SPDX-License-Identifier: GPL-3.0-or-later
#include <catch2/catch_test_macros.hpp>
#include "example-play-sample/PlayModel.hpp"

using namespace example_play;
using crosspad::EventSource;

TEST_CASE("play-sample: pads climb in semitones from the source", "[play-sample]")
{
    REQUIRE(transposeFor(0) == 0);
    REQUIRE(transposeFor(15) == 15);
}

TEST_CASE("play-sample: voice keys never collide with pad indexes", "[play-sample]")
{
    for (uint8_t p = 0; p < 16; ++p) REQUIRE(keyFor(p) >= 16);
    REQUIRE(keyFor(0) != keyFor(1));
}

TEST_CASE("play-sample: the loop always plays the kit", "[play-sample]")
{
    REQUIRE(kitPlays(Mode::Kit, EventSource::PadManager));
    REQUIRE(kitPlays(Mode::Pitched, EventSource::SequencerCallback));
    REQUIRE_FALSE(kitPlays(Mode::Pitched, EventSource::PadManager));
}
