// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>

namespace example_midi {

/// Pad 1 = C2 (36), the General MIDI drum map's kick, pad 16 = D#3 (51).
inline uint8_t noteFor(uint8_t pad) { return static_cast<uint8_t>(36 + (pad % 16)); }

/// Pressure becomes the mod wheel, CC 1.
constexpr uint8_t kPressureCc = 1;

/// The range onPadPan delivers: -64..63, centre 0 (the 1.x header says -127..127, but
/// the firmware already sends this). 2.0's IPadLogicHandler names them kPanMin/kPanMax.
constexpr int8_t kPanMin = -64;
constexpr int8_t kPanMax = 63;

/// A pad rocked left/right (kPanMin..kPanMax) bends the pitch (-8192..8191), the signed
/// range IMidiOutput::sendPitchBend takes (0 is centre, not 8192). Left is 128 steps per
/// pan unit (-64 -> -8192), right is scaled to end exactly on 8191 (63 -> 8191). Values
/// outside the range are clamped.
inline int16_t panToBend(int8_t pan)
{
    if (pan <= kPanMin) return -8192;
    if (pan >= kPanMax) return 8191;
    if (pan < 0) return static_cast<int16_t>(pan * 128);
    return static_cast<int16_t>(pan * 8191 / kPanMax);
}

/// Channels on the wire are 0..15; people count 1..16.
inline uint8_t nextChannel(uint8_t ch, int step) { return static_cast<uint8_t>((ch + 16 + step) % 16); }

} // namespace example_midi
