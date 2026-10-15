// SPDX-License-Identifier: GPL-3.0-or-later
#include "example-midi-out/MidiOutPadLogic.hpp"
#include "example-midi-out/MidiModel.hpp"
#include "crosspad/midi/IMidiOutput.hpp"
#include "crosspad/pad/PadManager.hpp"

using namespace example_midi;

// [docs:pack]
namespace {
// What the screen is told: status<<16 | data1<<8 | data2.
uint32_t pack(uint8_t status, uint8_t d1, uint8_t d2)
{
    return (uint32_t(status) << 16) | (uint32_t(d1) << 8) | d2;
}
}
// [/docs:pack]

void MidiOutPadLogic::onActivate(crosspad::PadManager& pm)
{
    lastMessage.store(0);   // a new visit starts with nothing sent
    for (uint8_t p = 0; p < 16; ++p) pm.setPadColor(p, 0, 40, 30);
}

// [docs:deactivate]
void MidiOutPadLogic::onDeactivate(crosspad::PadManager& pm)
{
    // Leaving the app with a pad down would leave its note sounding on the
    // synth: the release never reaches us once we are off the stack.
    const uint16_t held = held_.exchange(0);
    for (uint8_t p = 0; p < 16; ++p) {
        if (held & (1u << p)) endNote(p);
        pm.clearPadColor(p);
    }
}
// [/docs:deactivate]

// [docs:notes]
void MidiOutPadLogic::onPadPress(crosspad::PadManager& pm, uint8_t pad, uint8_t velocity)
{
    if (pad >= 16) return;
    const uint16_t bit = uint16_t(1u << pad);
    const uint8_t ch = channel.load();
    const uint8_t vel = velocity ? velocity : 1;   // velocity 0 would read as a Note Off
    // Pressed again without a release: end the old note first.
    if (held_.fetch_and(static_cast<uint16_t>(~bit)) & bit) endNote(pad);
    // Record the note before sending it, so the release and onDeactivate find it.
    // A real app may also send All Notes Off (CC 123) on close.
    noteChannel_[pad].store(ch);   // the release goes here, even if the screen changed the channel
    held_.fetch_or(bit);
    crosspad::getMidiOutput().sendNoteOn(noteFor(pad), vel, ch);
    pm.setPadColor(pad, 0, 220, 160);
    lastMessage.store(pack(0x90 | ch, noteFor(pad), vel));
}

void MidiOutPadLogic::onPadRelease(crosspad::PadManager& pm, uint8_t pad)
{
    if (pad >= 16) return;
    const uint16_t bit = uint16_t(1u << pad);
    if (!(held_.fetch_and(uint16_t(~bit)) & bit)) return;   // no Note On of ours is out
    endNote(pad);
    pm.setPadColor(pad, 0, 40, 30);
}

void MidiOutPadLogic::endNote(uint8_t pad)
{
    const uint8_t ch = noteChannel_[pad].load();
    crosspad::getMidiOutput().sendNoteOff(noteFor(pad), 0, ch);
    // Bend and CC are channel-wide, not per note: this example resets the bend on
    // every release and leaves CC 1 where it was, which suits one pad at a time.
    crosspad::getMidiOutput().sendPitchBend(0, ch);
    lastMessage.store(pack(0x80 | ch, noteFor(pad), 0));
}
// [/docs:notes]

// [docs:expression]
void MidiOutPadLogic::onPadPressure(crosspad::PadManager&, uint8_t pad, uint8_t pressure)
{
    if (pad >= 16 || !(held_.load() & (1u << pad))) return;   // a late event after the release
    const uint8_t ch = noteChannel_[pad].load();   // expression follows the note it belongs to
    crosspad::getMidiOutput().sendControlChange(kPressureCc, pressure, ch);
    lastMessage.store(pack(0xB0 | ch, kPressureCc, pressure));
}

void MidiOutPadLogic::onPadPan(crosspad::PadManager&, uint8_t pad, int8_t pan)
{
    if (pad >= 16 || !(held_.load() & (1u << pad))) return;   // a bend after the release detunes
    const uint8_t ch = noteChannel_[pad].load();
    const int16_t bend = panToBend(pan);
    crosspad::getMidiOutput().sendPitchBend(bend, ch);   // signed: 0 is centre
    const uint16_t raw = static_cast<uint16_t>(bend + 8192);   // the 14 wire bits, for the screen
    lastMessage.store(pack(0xE0 | ch, raw & 0x7F, raw >> 7));
}
// [/docs:expression]
