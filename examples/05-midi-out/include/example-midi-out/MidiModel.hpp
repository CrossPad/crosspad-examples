// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>

namespace example_midi {

/// Pad 1 = C2 (36), the General MIDI drum map's kick, pad 16 = D#3 (51).
inline uint8_t noteFor(uint8_t pad) { return static_cast<uint8_t>(36 + (pad % 16)); }

/// Pressure becomes the mod wheel, CC 1.
constexpr uint8_t kPressureCc = 1;

/// A pad rocked left/right (-127..127) bends the pitch (-8192..8191), the
/// signed range IMidiOutput::sendPitchBend takes (0 is centre, not 8192).
inline int16_t panToBend(int8_t pan)
{
    if (pan <= -127) return -8192;
    return static_cast<int16_t>(pan * 8191 / 127);
}

/// Channels on the wire are 0..15; people count 1..16.
inline uint8_t nextChannel(uint8_t ch, int step) { return static_cast<uint8_t>((ch + 16 + step) % 16); }

} // namespace example_midi
