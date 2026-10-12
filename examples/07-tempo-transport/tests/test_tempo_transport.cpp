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

TEST_CASE("tempo-transport: only the last four intervals count", "[tempo-transport]")
{
    TapTempo t;
    t.tap(0);
    REQUIRE(t.tap(1000) == Catch::Approx(60.0f));
    t.tap(1500); t.tap(2000);
    REQUIRE(t.tap(2500) == Catch::Approx(96.0f));   // 1000, 500, 500, 500
    REQUIRE(t.tap(3000) == Catch::Approx(120.0f));  // the 1000 ms interval has dropped out
}

TEST_CASE("tempo-transport: a gap of exactly two seconds still counts", "[tempo-transport]")
{
    TapTempo t;
    t.tap(0);
    REQUIRE(t.tap(2000) == Catch::Approx(30.0f));
    TapTempo u;
    u.tap(0);
    REQUIRE(u.tap(2001) == 0.0f);   // one millisecond more starts again
}

TEST_CASE("tempo-transport: a series ends after two seconds without a tap", "[tempo-transport]")
{
    TapTempo t;
    REQUIRE_FALSE(t.seriesEnded(100000));   // nothing tapped yet: nothing to end
    t.tap(0); t.tap(400);
    REQUIRE_FALSE(t.seriesEnded(2400));     // exactly two seconds after the last tap
    REQUIRE(t.seriesEnded(2401));
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
