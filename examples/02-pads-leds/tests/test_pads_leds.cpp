// SPDX-License-Identifier: GPL-3.0-or-later
#include <catch2/catch_test_macros.hpp>
#include "example-pads-leds/PadsModel.hpp"

using example_pads::padColor;

TEST_CASE("pads-leds: full velocity is the pure hue", "[pads-leds]")
{
    const auto c = padColor(0, 127);
    REQUIRE((c.r == 255 && c.g == 0 && c.b == 0));
}

TEST_CASE("pads-leds: zero is dark and over-range clamps", "[pads-leds]")
{
    const auto off = padColor(5, 0);
    REQUIRE((off.r == 0 && off.g == 0 && off.b == 0));
    const auto hi = padColor(5, 200), max = padColor(5, 127);
    REQUIRE((hi.r == max.r && hi.g == max.g && hi.b == max.b));
}

TEST_CASE("pads-leds: sixteen pads, sixteen different hues", "[pads-leds]")
{
    for (uint8_t a = 0; a < 16; ++a)
        for (uint8_t b = a + 1; b < 16; ++b) {
            const auto x = padColor(a, 127), y = padColor(b, 127);
            REQUIRE(!(x.r == y.r && x.g == y.g && x.b == y.b));
        }
}
