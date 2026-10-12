// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "crosspad/pad/IPadLogicHandler.hpp"
#include <atomic>
#include <cstdint>

// [docs:handler]
/// Pad callbacks run on the pad task, not the LVGL task: this class lights
/// the pads and records levels, and touches no LVGL object.
class PadsLedsPadLogic : public crosspad::IPadLogicHandler {
public:
    void onActivate(crosspad::PadManager& pm) override;
    void onDeactivate(crosspad::PadManager& pm) override;
    void onPadPress(crosspad::PadManager& pm, uint8_t pad, uint8_t velocity) override;
    void onPadRelease(crosspad::PadManager& pm, uint8_t pad) override;
    void onPadPressure(crosspad::PadManager& pm, uint8_t pad, uint8_t pressure) override;

    /// 0 = released; otherwise the latest velocity or pressure (1..127).
    uint8_t level(uint8_t pad) const { return pad < 16 ? levels_[pad].load() : 0; }
    int lastPad() const { return lastPad_.load(); }

private:
    std::atomic<uint8_t> levels_[16]{};
    std::atomic<int> lastPad_{-1};
};
// [/docs:handler]
