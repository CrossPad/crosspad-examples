// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "crosspad/pad/IPadLogicHandler.hpp"
#include <atomic>
#include <cstdint>

/// While this logic owns the pads the platform sends no MIDI for them:
/// every message below is this app's. Pad callbacks run on the pad task, so
/// this class touches no LVGL object.
class MidiOutPadLogic : public crosspad::IPadLogicHandler {
public:
    void onActivate(crosspad::PadManager& pm) override;
    void onDeactivate(crosspad::PadManager& pm) override;
    void onPadPress(crosspad::PadManager& pm, uint8_t pad, uint8_t velocity) override;
    void onPadRelease(crosspad::PadManager& pm, uint8_t pad) override;
    void onPadPressure(crosspad::PadManager& pm, uint8_t pad, uint8_t pressure) override;
    void onPadPan(crosspad::PadManager& pm, uint8_t pad, int8_t pan) override;

    std::atomic<uint8_t> channel{0};        // 0..15, set by the screen
    std::atomic<uint32_t> lastMessage{0};   // status<<16 | data1<<8 | data2, for the screen

private:
    void endNote(uint8_t pad);              // Note Off and bend reset, on the channel the note started on

    std::atomic<uint16_t> held_{0};         // bit n set = pad n has a Note On out
    std::atomic<uint8_t> noteChannel_[16]{};
};
