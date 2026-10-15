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
    flushPitched(pm);   // no pitched note outlives the app
    if (auto* b = base_.exchange(nullptr)) b->onDeactivate(pm);
}
// [/docs:base]

// [docs:flush]
// One place ends a pitched note: the release and the app closing both use it.
// The off reaches the engine; what it ends depends on the pad's mode in the kit.
void PlaySamplePadLogic::endPitched(crosspad::PadManager& pm, uint8_t pad)
{
    crosspad::getEventBus().postSamplePlayback(kSourcePad, /*wavIdx=*/0, /*isOn=*/false,
                                               crosspad::EventSource::App, 127,
                                               transposeFor(pad), keyFor(pad));
    pm.notifyNoteOff(pad, /*fromSampler=*/true);
}

// A note still held when the app closes would never get its off.
void PlaySamplePadLogic::flushPitched(crosspad::PadManager& pm)
{
    const uint16_t held = pitchedHeld_.exchange(0);
    for (uint8_t p = 0; p < 16; ++p) {
        if (held & (1u << p)) endPitched(pm, p);
    }
}
// [/docs:flush]

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
    // The loop's release always belongs to the kit, even while a finger holds
    // this pad in Pitched mode: the manager stores a release's source before it
    // calls us, so pressSource() says who let go.
    if (pm.pressSource(pad) == crosspad::EventSource::SequencerCallback) {
        if (auto* b = base_.load()) b->onPadRelease(pm, pad);
        return;
    }
    // A finger's release goes to whoever played that press: the held bit says
    // whether it was ours (pitched) or the kit's, even if the mode changed since.
    const uint16_t bit = static_cast<uint16_t>(1u << pad);
    if (pitchedHeld_.fetch_and(static_cast<uint16_t>(~bit)) & bit) {
        endPitched(pm, pad);
    } else if (auto* b = base_.load()) {
        b->onPadRelease(pm, pad);
    }
}
// [/docs:press]

void PlaySamplePadLogic::onPadPressure(crosspad::PadManager& pm, uint8_t pad, uint8_t pressure)
{
    if (auto* b = base_.load()) b->onPadPressure(pm, pad, pressure);
}
