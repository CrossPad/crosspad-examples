// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <cstdio>
#include <string>

namespace example_ui {

/// The toast a pad press posts: "Pad 1", "Pad 16".
inline std::string padToast(uint8_t padIdx)
{
    char buf[16];
    std::snprintf(buf, sizeof buf, "Pad %u", static_cast<unsigned>(padIdx) + 1u);
    return buf;
}

/// Pack a pad index for postToGui's void* -- no allocation, nothing to free.
/// The index is stored plus one, so pad 0 is not a null pointer.
inline void* packPad(uint8_t padIdx)
{
    return reinterpret_cast<void*>(static_cast<uintptr_t>(padIdx) + 1u);
}

inline uint8_t unpackPad(void* p)
{
    return static_cast<uint8_t>(reinterpret_cast<uintptr_t>(p) - 1u);
}

} // namespace example_ui
