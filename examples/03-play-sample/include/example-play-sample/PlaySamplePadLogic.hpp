// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "crosspad/pad/IPadLogicHandler.hpp"
#include "example-play-sample/PlayModel.hpp"
#include <atomic>

/// Kit mode: every call goes to the base pad logic -- the kit plays as on
/// the Sampler. Pitched mode: this class plays the source pad's sample itself.
class PlaySamplePadLogic : public crosspad::IPadLogicHandler {
public:
    void onActivate(crosspad::PadManager& pm) override;
    void onDeactivate(crosspad::PadManager& pm) override;
    void onPadPress(crosspad::PadManager& pm, uint8_t pad, uint8_t velocity) override;
    void onPadRelease(crosspad::PadManager& pm, uint8_t pad) override;
    void onPadPressure(crosspad::PadManager& pm, uint8_t pad, uint8_t pressure) override;

    void setMode(example_play::Mode m) { mode_.store(m); }
    example_play::Mode mode() const { return mode_.load(); }

private:
    std::atomic<crosspad::IPadLogicHandler*> base_{nullptr};
    std::atomic<example_play::Mode> mode_{example_play::Mode::Kit};
    std::atomic<uint16_t> pitchedHeld_{0};   // bit per pad held in pitched mode
};
