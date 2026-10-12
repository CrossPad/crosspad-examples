// SPDX-License-Identifier: GPL-3.0-or-later
#include "example-pads-leds/PadsLedsPadLogic.hpp"
#include "example-pads-leds/PadsModel.hpp"
#include "crosspad/pad/PadManager.hpp"

using example_pads::padColor;

namespace {
// [docs:light]
void light(crosspad::PadManager& pm, uint8_t pad, uint8_t level)
{
    const auto c = padColor(pad, level);
    pm.setPadColor(pad, c.r, c.g, c.b);
}
// [/docs:light]
} // namespace

// [docs:activate]
void PadsLedsPadLogic::onActivate(crosspad::PadManager& pm)
{
    for (uint8_t p = 0; p < 16; ++p) {
        levels_[p].store(0);
        light(pm, p, example_pads::kRestLevel);
    }
    lastPad_.store(-1);        // a new visit starts with no "last pad"
}

void PadsLedsPadLogic::onDeactivate(crosspad::PadManager& pm)
{
    // Hand the LEDs back: the next pad logic paints its own colours.
    for (uint8_t p = 0; p < 16; ++p) pm.clearPadColor(p);
}
// [/docs:activate]

// [docs:press]
void PadsLedsPadLogic::onPadPress(crosspad::PadManager& pm, uint8_t pad, uint8_t velocity)
{
    if (pad >= 16) return;
    const uint8_t v = velocity ? velocity : 1;
    light(pm, pad, v);
    levels_[pad].store(v);     // the screen reads these on the LVGL task
    lastPad_.store(pad);
}

void PadsLedsPadLogic::onPadPressure(crosspad::PadManager& pm, uint8_t pad, uint8_t pressure)
{
    if (pad >= 16 || levels_[pad].load() == 0) return;   // aftertouch only while held
    const uint8_t v = pressure ? pressure : 1;
    light(pm, pad, v);
    levels_[pad].store(v);
}

void PadsLedsPadLogic::onPadRelease(crosspad::PadManager& pm, uint8_t pad)
{
    if (pad >= 16) return;
    light(pm, pad, example_pads::kRestLevel);
    levels_[pad].store(0);
}
// [/docs:press]
