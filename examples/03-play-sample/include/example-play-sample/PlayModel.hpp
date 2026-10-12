// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include "crosspad/event/EventTypes.hpp"

namespace example_play {

enum class Mode : uint8_t { Kit, Pitched };

/// Pitched mode plays the sample of this pad on every pad.
constexpr uint8_t kSourcePad = 0;

/// Semitones above the source for pad @p pad: pad 1 plays it as recorded,
/// pad 16 fifteen semitones up.
inline int8_t transposeFor(uint8_t pad) { return static_cast<int8_t>(pad); }

/// The voice key: two pads playing one sample at two pitches are two notes.
/// 16.. stays clear of the pad indexes 0..15 the kit's own voices use.
inline uint8_t keyFor(uint8_t pad) { return static_cast<uint8_t>(16 + pad); }

/// Who plays a press: the kit (the base pad logic) or this app.
/// The loop's own presses always go to the kit, so a pattern keeps playing
/// what it recorded.
inline bool kitPlays(Mode mode, crosspad::EventSource source)
{
    return mode == Mode::Kit || source == crosspad::EventSource::SequencerCallback;
}

} // namespace example_play
