// SPDX-License-Identifier: GPL-3.0-or-later
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "example-tempo-transport/TempoModel.hpp"

using namespace example_tempo;

TEST_CASE("tempo-transport: steady taps give their tempo", "[tempo-transport]")
{
    TapTempo t;
    REQUIRE(t.tap(1000) == 0.0f);
    REQUIRE(t.tap(1500) == Catch::Approx(120.0f));
    REQUIRE(t.tap(2000) == Catch::Approx(120.0f));
}

TEST_CASE("tempo-transport: a long pause starts again", "[tempo-transport]")
{
    TapTempo t;
    t.tap(0); t.tap(500);
    REQUIRE(t.tap(5000) == 0.0f);
    REQUIRE(t.tap(5250) == Catch::Approx(240.0f));
}

TEST_CASE("tempo-transport: taps clamp to the transport's limits", "[tempo-transport]")
{
    TapTempo t;
    t.tap(0);
    REQUIRE(t.tap(100) == Catch::Approx(crosspad::kTransportTempoMax));
}

TEST_CASE("tempo-transport: the target plays and stops", "[tempo-transport]")
{
    BlinkTarget b;
    REQUIRE(b.state() == crosspad::TransportState::Stopped);
    b.play();
    REQUIRE(b.state() == crosspad::TransportState::Playing);
    REQUIRE(b.caps() == (crosspad::kTransportCanPlay | crosspad::kTransportCanStop));
    b.stop();
    REQUIRE(b.state() == crosspad::TransportState::Stopped);
}

TEST_CASE("tempo-transport: beats count round the bar", "[tempo-transport]")
{
    REQUIRE(beatAt(0, 120.0f) == 0);
    REQUIRE(beatAt(500, 120.0f) == 1);
    REQUIRE(beatAt(2000, 120.0f) == 0);
}
