// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <atomic>
#include <cstdint>
#include "crosspad/transport/Transport.hpp"

namespace example_tempo {

/// Tap tempo: the average of the last four intervals. A gap over two seconds
/// starts a new count. Pure: the caller passes the time.
class TapTempo {
public:
    /// @return the tempo in BPM once two taps are in, else 0.
    float tap(uint32_t nowMs)
    {
        if (count_ > 0 && nowMs - last_ > 2000) count_ = 0;
        if (count_ > 0) {
            intervals_[(count_ - 1) % 4] = nowMs - last_;
        }
        last_ = nowMs;
        ++count_;
        const int n = count_ - 1 < 4 ? count_ - 1 : 4;
        if (n == 0) return 0.0f;
        uint32_t sum = 0;
        for (int i = 0; i < n; ++i) sum += intervals_[i];
        float bpm = 60000.0f * n / static_cast<float>(sum);
        if (bpm < crosspad::kTransportTempoMin) bpm = crosspad::kTransportTempoMin;
        if (bpm > crosspad::kTransportTempoMax) bpm = crosspad::kTransportTempoMax;
        return bpm;
    }

private:
    uint32_t intervals_[4]{};
    uint32_t last_ = 0;
    int count_ = 0;
};

/// The smallest transport target: PLAY and STOP on the bar start and stop a
/// beat that this app shows. Everything else keeps the interface's defaults.
// [docs:target]
class BlinkTarget : public crosspad::ITransportTarget {
public:
    void play() override { playing_.store(true); }
    void stop() override { playing_.store(false); }
    crosspad::TransportState state() const override
    {
        return playing_.load() ? crosspad::TransportState::Playing : crosspad::TransportState::Stopped;
    }
    const char* name() const override { return "BLINK"; }

private:
    // The bar reads it on the LVGL task; PLAY can also arrive from a host's
    // MIDI Start on another task. An atomic keeps state() cheap and safe.
    std::atomic<bool> playing_{false};
};
// [/docs:target]

/// Which beat of the bar (0..3) is sounding @p ms after PLAY at @p bpm.
inline int beatAt(uint32_t ms, float bpm)
{
    if (bpm <= 0.0f) return 0;
    const uint32_t beatMs = static_cast<uint32_t>(60000.0f / bpm);
    return static_cast<int>((ms / beatMs) % 4);
}

} // namespace example_tempo
