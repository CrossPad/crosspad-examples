// SPDX-License-Identifier: GPL-3.0-or-later
#include "example-midi-out/MidiOutPadLogic.hpp"
#include "example-midi-out/MidiModel.hpp"
#include "crosspad/midi/IMidiOutput.hpp"
#include "crosspad/pad/PadManager.hpp"

using namespace example_midi;

namespace {
uint32_t pack(uint8_t status, uint8_t d1, uint8_t d2) { return (uint32_t(status) << 16) | (uint32_t(d1) << 8) | d2; }
}

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
    const uint8_t ch = channel.load(), vel = velocity ? velocity : 1;   // velocity 0 would read as a Note Off
    crosspad::getMidiOutput().sendNoteOn(noteFor(pad), vel, ch);
    noteChannel_[pad].store(ch);   // the release goes to this channel, even if the screen changed it since
    held_.fetch_or(uint16_t(1u << pad));
    pm.setPadColor(pad, 0, 220, 160);
    lastMessage.store(pack(0x90 | ch, noteFor(pad), vel));
}

void MidiOutPadLogic::onPadRelease(crosspad::PadManager& pm, uint8_t pad)
{
    if (pad >= 16) return;
    const uint16_t bit = uint16_t(1u << pad);
    if (!(held_.fetch_and(uint16_t(~bit)) & bit)) return;   // no Note On of ours is out: nothing to end
    endNote(pad);
    pm.setPadColor(pad, 0, 40, 30);
}

void MidiOutPadLogic::endNote(uint8_t pad)
{
    const uint8_t ch = noteChannel_[pad].load();
    crosspad::getMidiOutput().sendNoteOff(noteFor(pad), 0, ch);
    crosspad::getMidiOutput().sendPitchBend(0, ch);   // a bend ends with its note
    lastMessage.store(pack(0x80 | ch, noteFor(pad), 0));
}
// [/docs:notes]

// [docs:expression]
void MidiOutPadLogic::onPadPressure(crosspad::PadManager&, uint8_t pad, uint8_t pressure)
{
    if (pad >= 16) return;
    const uint8_t ch = noteChannel_[pad].load();   // expression follows the note it belongs to
    crosspad::getMidiOutput().sendControlChange(kPressureCc, pressure, ch);
    lastMessage.store(pack(0xB0 | ch, kPressureCc, pressure));
}

void MidiOutPadLogic::onPadPan(crosspad::PadManager&, uint8_t pad, int8_t pan)
{
    if (pad >= 16) return;
    const uint8_t ch = noteChannel_[pad].load();
    const int16_t bend = panToBend(pan);
    crosspad::getMidiOutput().sendPitchBend(bend, ch);   // signed: 0 is centre
    const uint16_t raw = static_cast<uint16_t>(bend + 8192);   // the 14 bits on the wire, for the screen
    lastMessage.store(pack(0xE0 | ch, raw & 0x7F, raw >> 7));
}
// [/docs:expression]
