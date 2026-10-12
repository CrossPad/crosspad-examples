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
#include "example-tempo-transport/TempoModel.hpp"
#include "crosspad/transport/Transport.hpp"
#include "crosspad/event/IEventBus.hpp"
#include "crosspad/event/EventData.hpp"
#include "crosspad/pad/PadManager.hpp"
#include "crosspad/platform/IClock.hpp"
#include "crosspad-gui/components/transport_bar.h"
#include <atomic>

/*
 * Tempo and Transport -- PLAY and STOP on the transport bar drive this app,
 * pads 1-4 count the beat, Tap sets the tempo for everything.
 * Recipe: https://crosspad.app/docs/apps/cookbook/tempo-transport/
 */
namespace {

// [docs:state]
example_tempo::BlinkTarget s_target;               // what PLAY and STOP reach while we hold the claim
example_tempo::TapTempo s_tap;                     // the taps so far
std::atomic<float> s_lastChange{0.0f};            // the last TempoChanged, written on the event task
crosspad::SubscriptionHandle s_sub = nullptr;      // our TempoChanged subscription
lv_timer_t* s_timer = nullptr;                     // paints the labels and the pads
lv_group_t* s_group = nullptr;                     // what the encoder walks
lv_obj_t* s_bpm = nullptr;
lv_obj_t* s_beat = nullptr;
lv_obj_t* s_from = nullptr;
uint32_t s_playStartMs = 0;                        // when this PLAY began
bool s_wasPlaying = false;
int s_shownBeat = -2;                              // -2: nothing painted yet, -1: stopped
// [/docs:state]

uint32_t now_ms() { return static_cast<uint32_t>(crosspad::getClock().getTimeUs() / 1000); }

// [docs:subscribe]
// Runs on the event task, and the data lives only during this call:
// copy what you need somewhere the LVGL task can read it.
void on_tempo(const void* data, crosspad::EventType, void*)
{
    s_lastChange.store(static_cast<const crosspad::TempoChangedData*>(data)->bpm);
}
// [/docs:subscribe]

// [docs:tap]
void on_tap(lv_event_t*)
{
    const float bpm = s_tap.tap(now_ms());
    if (bpm <= 0.0f) return;                        // the first tap only starts the count
    if (crosspad::transportTempoFromHost()) return; // a host's MIDI clock would undo it
    crosspad::transportSetTempo(bpm);               // heard at once
    crosspad::transportCommitTempo();               // the edit is done: an open song takes it too
    crosspad_gui::transportbar_refresh();
}
// [/docs:tap]

void light_beat(int beat)
{
    auto& pads = crosspad::getPadManager();
    for (uint8_t p = 0; p < 4; ++p) {
        if (p == beat) pads.setPadColor(p, 255, 200, 40);
        else           pads.setPadColor(p, 30, 22, 4);
    }
}

// [docs:timer]
void tick(lv_timer_t*)
{
    const float bpm = crosspad::transportTempo();
    if (bpm > 0.0f) lv_label_set_text_fmt(s_bpm, "%d BPM", static_cast<int>(bpm + 0.5f));
    else            lv_label_set_text(s_bpm, "No tempo yet");
    if (crosspad::transportTempoFromHost()) lv_label_set_text(s_from, "The host's MIDI clock sets the tempo");
    else if (s_lastChange.load() > 0.0f)    lv_label_set_text_fmt(s_from, "Last change: %d BPM", static_cast<int>(s_lastChange.load() + 0.5f));

    const bool playing = s_target.state() == crosspad::TransportState::Playing;
    if (playing && !s_wasPlaying) s_playStartMs = now_ms();
    s_wasPlaying = playing;
    const int beat = playing ? example_tempo::beatAt(now_ms() - s_playStartMs, bpm) : -1;
    if (beat == s_shownBeat) return;
    s_shownBeat = beat;
    light_beat(beat);
    if (beat >= 0) lv_label_set_text_fmt(s_beat, "Beat %d", beat + 1);
    else           lv_label_set_text(s_beat, "Press PLAY on the transport bar");
}
// [/docs:timer]

} // namespace

lv_obj_t* TempoTransport_create(lv_obj_t* parent, App*)
{
    // Every static starts clean on each visit.
    s_target.stop();
    s_tap = example_tempo::TapTempo{};
    s_lastChange.store(0.0f);
    s_sub = nullptr;
    s_timer = nullptr;
    s_group = nullptr;
    s_bpm = s_beat = s_from = nullptr;
    s_playStartMs = 0;
    s_wasPlaying = false;
    s_shownBeat = -2;   // paint on the first tick

    if (lv_indev_t* enc = crosspad_gui::getGuiPlatform().getNavigationEncoder()) {
        s_group = lv_group_create();
        lv_indev_set_group(enc, s_group);
    }

    lv_obj_t* root = lv_obj_create(parent);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(root, 10, 0);
    s_bpm = lv_label_create(root);
    s_beat = lv_label_create(root);
    s_from = lv_label_create(root);
    lv_label_set_text(s_from, "");

    lv_obj_t* tap = lv_button_create(root);
    lv_obj_add_event_cb(tap, on_tap, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* l = lv_label_create(tap);
    lv_label_set_text(l, "Tap");
    if (s_group) lv_group_add_obj(s_group, tap);

    // [docs:claim]
    // From now on PLAY means this app. A claim outlives the screen on purpose
    // (the Sequencer keeps playing when you leave it); this target has nothing
    // to play without its screen, so destroy stops and releases it.
    crosspad::transportClaim(&s_target);
    crosspad_gui::transportbar_refresh();
    // [/docs:claim]

    s_sub = crosspad::getEventBus().subscribe(crosspad::EventType::TempoChanged, on_tempo, nullptr);
    s_timer = lv_timer_create(tick, 33, nullptr);
    return root;
}

// [docs:destroy]
void TempoTransport_destroy(lv_obj_t* root)
{
    if (s_timer) { lv_timer_delete(s_timer); s_timer = nullptr; }
    if (s_sub) { crosspad::getEventBus().unsubscribe(s_sub); s_sub = nullptr; }
    s_target.stop();
    crosspad::transportRelease(&s_target);   // a no-op if another app has claimed since
    crosspad_gui::transportbar_refresh();
    for (uint8_t p = 0; p < 4; ++p) crosspad::getPadManager().clearPadColor(p);
    if (s_group) { lv_group_delete_deferred(s_group); s_group = nullptr; }
    s_bpm = s_beat = s_from = nullptr;
    lv_obj_delete_async(root);
}
// [/docs:destroy]

// No pad logic: the pads keep playing the kit; this app only colours pads 1-4.
REGISTER_APP(TempoTransport, nullptr, "example-tempo-transport.png",
             TempoTransport_create, TempoTransport_destroy,
             nullptr, nullptr, nullptr, 0)
