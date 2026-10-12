// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>

namespace example_pads {

struct Rgb { uint8_t r, g, b; };

/// One hue per pad, round the colour wheel: pad 0 red, then on through
/// yellow, green, cyan, blue and magenta. Integer math only.
inline Rgb wheel(uint8_t pad)
{
    const uint16_t h = static_cast<uint16_t>(pad % 16) * 96;   // 0..1440 of 1536
    const uint8_t f = static_cast<uint8_t>(h % 256);
    switch (h / 256) {
    case 0:  return {255, f, 0};
    case 1:  return {static_cast<uint8_t>(255 - f), 255, 0};
    case 2:  return {0, 255, f};
    case 3:  return {0, static_cast<uint8_t>(255 - f), 255};
    case 4:  return {f, 0, 255};
    default: return {255, 0, static_cast<uint8_t>(255 - f)};
    }
}

/// The pad's hue at a brightness of @p level (0..127: a velocity or a pressure).
inline Rgb padColor(uint8_t pad, uint8_t level)
{
    if (level > 127) level = 127;
    const Rgb c = wheel(pad);
    return {static_cast<uint8_t>(c.r * level / 127), static_cast<uint8_t>(c.g * level / 127),
            static_cast<uint8_t>(c.b * level / 127)};
}

/// Pads at rest glow at this level, so the hues show before the first hit.
constexpr uint8_t kRestLevel = 12;

} // namespace example_pads
