// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdio>
#include <string>

namespace example_hello {

/// The line under the button: "Pressed 1 time", "Pressed 3 times".
inline std::string pressedLabel(int count)
{
    char buf[32];
    std::snprintf(buf, sizeof buf, "Pressed %d %s", count, count == 1 ? "time" : "times");
    return buf;
}

} // namespace example_hello
