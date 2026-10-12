// SPDX-License-Identifier: GPL-3.0-or-later
#include <catch2/catch_test_macros.hpp>
#include "example-ui-kit/UiModel.hpp"

using namespace example_ui;

TEST_CASE("ui-kit: toasts count pads from 1", "[ui-kit]")
{
    REQUIRE(padToast(0) == "Pad 1");
    REQUIRE(padToast(15) == "Pad 16");
}

TEST_CASE("ui-kit: a pad survives the trip through void*", "[ui-kit]")
{
    for (uint8_t p = 0; p < 16; ++p) REQUIRE(unpackPad(packPad(p)) == p);
    REQUIRE(packPad(0) != nullptr);
}
