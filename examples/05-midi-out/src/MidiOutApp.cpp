// SPDX-License-Identifier: GPL-3.0-or-later
#if __has_include("pc_stubs/PcApp.hpp")
  #include "pc_stubs/PcApp.hpp"        // the simulator's App
#else
  #include "crosspad-gui/app/App.hpp"  // the firmware's App
#endif
#include "crosspad/app/AppRegistrar.hpp"
#include "crosspad-gui/platform/IGuiPlatform.h"
#include "crosspad-gui/widgets/gui_helpers.h"
#include "lvgl.h"
#include "example-midi-out/MidiOutPadLogic.hpp"
#include "example-midi-out/MidiModel.hpp"
#include "crosspad/pad/PadManager.hpp"
#include <memory>

/*
 * MIDI Out -- the pads as a MIDI controller: notes, pressure as CC 1, a pad
 * rocked left or right as pitch bend.
 * Recipe: https://crosspad.app/docs/apps/cookbook/midi-out/
 */
namespace {

// [docs:state]
std::shared_ptr<MidiOutPadLogic> s_logic;   // kept for the life of the program (see create)
lv_obj_t* s_last = nullptr;      // the last message sent, spelled out
lv_obj_t* s_channel = nullptr;   // "Channel 1".."Channel 16"
lv_timer_t* s_timer = nullptr;   // draws what the pad task recorded
lv_group_t* s_group = nullptr;   // what the encoder walks
uint32_t s_shown = 0;            // the last message the label shows
// [/docs:state]

const char* const kNote[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

// [docs:timer]
// The pad task packed the last message into an atomic; this draws it.
void tick(lv_timer_t*)
{
    const uint32_t m = s_logic->lastMessage.load();
    if (m == s_shown) return;
    s_shown = m;
    const unsigned status = m >> 16, d1 = (m >> 8) & 0x7F, d2 = m & 0x7F, ch = (status & 0x0F) + 1;
    switch (status & 0xF0) {
    case 0x90: lv_label_set_text_fmt(s_last, "Note On  %s%d (%u)  vel %u  ch %u", kNote[d1 % 12], int(d1 / 12) - 1, d1, d2, ch); break;
    case 0x80: lv_label_set_text_fmt(s_last, "Note Off  %s%d (%u)  ch %u", kNote[d1 % 12], int(d1 / 12) - 1, d1, ch); break;
    case 0xB0: lv_label_set_text_fmt(s_last, "CC %u = %u  ch %u", d1, d2, ch); break;
    case 0xE0: lv_label_set_text_fmt(s_last, "Pitch bend %d  ch %u", int((d2 << 7) | d1) - 8192, ch); break;
    default: lv_label_set_text(s_last, "Nothing sent yet"); break;   // lastMessage was reset to 0
    }
}
// [/docs:timer]

void show_channel() { lv_label_set_text_fmt(s_channel, "Channel %u", s_logic->channel.load() + 1u); }
void on_down(lv_event_t*) { s_logic->channel.store(example_midi::nextChannel(s_logic->channel.load(), -1)); show_channel(); }
void on_up(lv_event_t*)   { s_logic->channel.store(example_midi::nextChannel(s_logic->channel.load(), +1)); show_channel(); }

lv_obj_t* add_button(lv_obj_t* parent, const char* text, lv_event_cb_t cb)
{
    lv_obj_t* b = lv_button_create(parent);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* l = lv_label_create(b);
    lv_label_set_text(l, text);
    lv_obj_center(l);
    if (s_group) lv_group_add_obj(s_group, b);
    return b;
}

} // namespace

lv_obj_t* MidiOut_create(lv_obj_t* parent, App*)
{
    // Every static starts clean on each visit -- except s_logic, on purpose.
    s_last = s_channel = nullptr;
    s_timer = nullptr;
    s_group = nullptr;
    s_shown = 0;

    // Registered once and kept: REGISTER_APP_PL below makes the framework
    // push it when the app opens and pop it when the app closes.
    if (!s_logic) {
        s_logic = std::make_shared<MidiOutPadLogic>();
        crosspad::getPadManager().registerPadLogic("MidiOut", s_logic);
    }
    s_logic->channel.store(0);   // every visit starts on channel 1 (0 on the wire)

    if (lv_indev_t* enc = crosspad_gui::getGuiPlatform().getNavigationEncoder()) {
        s_group = lv_group_create();
        lv_indev_set_group(enc, s_group);
    }

    lv_obj_t* root = lv_obj_create(parent);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(root, 10, 0);

    lv_obj_t* title = lv_label_create(root);
    lv_label_set_text(title, "Hit, press or rock a pad");
    s_last = lv_label_create(root);
    lv_obj_set_width(s_last, LV_PCT(100));
    lv_label_set_text(s_last, "Nothing sent yet");

    lv_obj_t* row = lv_obj_create(root);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 12, 0);
    add_button(row, "Ch -", on_down);
    s_channel = lv_label_create(row);
    add_button(row, "Ch +", on_up);
    show_channel();

    lv_obj_t* where = lv_label_create(root);
    lv_obj_set_width(where, LV_PCT(100));
    lv_label_set_text(where, "Goes out over USB, Bluetooth and the\nSTM32 port, as the MIDI settings route it.");

    s_timer = lv_timer_create(tick, 33, nullptr);
    return root;
}

void MidiOut_destroy(lv_obj_t* root)
{
    if (s_timer) { lv_timer_delete(s_timer); s_timer = nullptr; }
    if (s_group) { lv_group_delete_deferred(s_group); s_group = nullptr; }
    s_last = s_channel = nullptr;
    lv_obj_delete_async(root);
}

// [docs:register]
REGISTER_APP_PL(MidiOut, nullptr, "example-midi-out.png",
                MidiOut_create, MidiOut_destroy,
                nullptr, nullptr, nullptr, 0, "MidiOut")
// [/docs:register]
