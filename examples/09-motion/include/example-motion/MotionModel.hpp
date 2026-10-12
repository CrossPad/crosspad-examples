// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>

namespace example_motion {

/// Tilt becomes CC 74 (brightness/cutoff on most synths).
constexpr uint8_t kTiltCc = 74;

/// Roll in degrees, -45..45, to a CC value 0..127; past 45 degrees it holds.
inline uint8_t tiltToCc(float rollDeg)
{
    if (rollDeg <= -45.0f) return 0;
    if (rollDeg >= 45.0f) return 127;
    return static_cast<uint8_t>((rollDeg + 45.0f) * 127.0f / 90.0f + 0.5f);
}

/// Send only when the value moved by at least 2: the sensor jitters by one.
inline bool worthSending(int last, uint8_t now)
{
    return last < 0 || now > last + 1 || now + 1 < last;
}

} // namespace example_motion
