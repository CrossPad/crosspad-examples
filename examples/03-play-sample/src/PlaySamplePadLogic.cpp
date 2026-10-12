// SPDX-License-Identifier: GPL-3.0-or-later
#include "example-play-sample/PlaySamplePadLogic.hpp"
#include "crosspad/event/IEventBus.hpp"
#include "crosspad/pad/PadManager.hpp"

using namespace example_play;

// [docs:base]
void PlaySamplePadLogic::onActivate(crosspad::PadManager& pm)
{
    pitchedHeld_.store(0);   // a new visit starts with no pad held
    // The base pad logic is what plays the kit when no app owns the pads.
    auto base = pm.getPadLogic(pm.getBasePadLogic());
    crosspad::IPadLogicHandler* b = (base.get() == this) ? nullptr : base.get();
    if (b) b->onActivate(pm);
    base_.store(b);
}

void PlaySamplePadLogic::onDeactivate(crosspad::PadManager& pm)
{
    // A pitched note still held when the app closes would never get its off.
    const uint16_t held = pitchedHeld_.exchange(0);
    for (uint8_t p = 0; p < 16; ++p) {
        if (!(held & (1u << p))) continue;
        crosspad::getEventBus().postSamplePlayback(kSourcePad, 0, /*isOn=*/false,
                                                   crosspad::EventSource::App, 127,
                                                   transposeFor(p), keyFor(p));
        pm.notifyNoteOff(p, /*fromSampler=*/true);
    }
    if (auto* b = base_.exchange(nullptr)) b->onDeactivate(pm);
}
// [/docs:base]

// [docs:press]
void PlaySamplePadLogic::onPadPress(crosspad::PadManager& pm, uint8_t pad, uint8_t velocity)
{
    if (pad >= 16) return;
    if (kitPlays(mode_.load(), pm.pressSource(pad))) {
        if (auto* b = base_.load()) b->onPadPress(pm, pad, velocity);
        return;
    }
    crosspad::getEventBus().postSamplePlayback(kSourcePad, /*wavIdx=*/0, /*isOn=*/true,
                                               crosspad::EventSource::App, velocity ? velocity : 1,
                                               transposeFor(pad), keyFor(pad));
    pm.notifyNoteOn(pad, velocity, /*fromSampler=*/true);   // the pad lights as on the Sampler
    pitchedHeld_.fetch_or(static_cast<uint16_t>(1u << pad));
}

void PlaySamplePadLogic::onPadRelease(crosspad::PadManager& pm, uint8_t pad)
{
    if (pad >= 16) return;
    const uint16_t bit = static_cast<uint16_t>(1u << pad);
    if (!(pitchedHeld_.fetch_and(static_cast<uint16_t>(~bit)) & bit)) {
        if (auto* b = base_.load()) b->onPadRelease(pm, pad);   // a kit press: the kit releases it
        return;
    }
    // A pad in Hold mode plays until released; one-shot pads ignore the off.
    crosspad::getEventBus().postSamplePlayback(kSourcePad, 0, /*isOn=*/false,
                                               crosspad::EventSource::App, 127,
                                               transposeFor(pad), keyFor(pad));
    pm.notifyNoteOff(pad, /*fromSampler=*/true);
}
// [/docs:press]

void PlaySamplePadLogic::onPadPressure(crosspad::PadManager& pm, uint8_t pad, uint8_t pressure)
{
    if (auto* b = base_.load()) b->onPadPressure(pm, pad, pressure);
}
