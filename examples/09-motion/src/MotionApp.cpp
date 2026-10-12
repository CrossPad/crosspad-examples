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
#include "example-motion/MotionModel.hpp"
#include "crosspad/event/IEventBus.hpp"
#include "crosspad/event/EventData.hpp"
#include "crosspad/platform/IImu.hpp"
#include "crosspad/platform/IVibration.hpp"
#include "crosspad/platform/VibrationPreset.hpp"
#include "crosspad/midi/IMidiOutput.hpp"
#include <atomic>

/*
 * Motion -- tilt the CrossPad to send CC 74, tap it to feel it tap back.
 * Recipe: https://crosspad.app/docs/apps/cookbook/motion/
 */
namespace {

// [docs:state]
std::atomic<int> s_taps{0};                    // taps and double taps, counted on the event task
std::atomic<int> s_gesture{-1};                // the last MotionGesture seen, -1 before any
crosspad::SubscriptionHandle s_sub = nullptr;  // our MotionGesture subscription
lv_timer_t* s_timer = nullptr;                 // polls the sensor and paints the labels
lv_obj_t* s_dot = nullptr;                     // the spirit level's dot
lv_obj_t* s_cc = nullptr;
lv_obj_t* s_info = nullptr;
int s_lastCc = -1;                             // the last CC value sent, -1 before the first
int s_shownTaps = -1;                          // what s_info shows now, to repaint on change only
int s_shownGesture = -2;
int s_shownSensor = -1;                        // -1 unknown, 0 no sensor, 1 sensor
// [/docs:state]

// [docs:helpers]
const char* gesture_name(int g)
{
    switch (static_cast<crosspad::MotionGesture>(g)) {
    case crosspad::MotionGesture::SingleTap:   return "tap";
    case crosspad::MotionGesture::DoubleTap:   return "double tap";
    case crosspad::MotionGesture::FreeFall:    return "free fall";
    case crosspad::MotionGesture::WakeUp:      return "wake up";
    case crosspad::MotionGesture::Orientation: return "turned";
    case crosspad::MotionGesture::Still:       return "still";
    }
    return "-";
}

// The spirit level: a square box with a 1 px border, a dot inside it. The dot's
// top-left corner moves kHalf either way from the middle, so it never leaves the box.
constexpr int32_t kBox = 120;                       // box side, border included
constexpr int32_t kDot = 10;                        // dot side
constexpr int32_t kHalf = (kBox - 2 - kDot) / 2;    // 54: travel from the middle
constexpr float kMaxDeg = 45.0f;                    // the angle that reaches the edge

// Keeps a tilt angle inside the spirit level.
float edge(float deg) { return deg < -kMaxDeg ? -kMaxDeg : (deg > kMaxDeg ? kMaxDeg : deg); }

// Where the dot's top-left corner goes for a tilt in degrees.
int32_t dot_pos(float deg) { return kHalf + static_cast<int32_t>(edge(deg) * kHalf / kMaxDeg); }
// [/docs:helpers]

// [docs:gesture]
// Runs on the event task, so LVGL is off limits: the counts go through atomics.
// vibrate() only queues the preset for the haptics task (it does not wait for
// the motor), so it can be called from here.
void on_gesture(const void* data, crosspad::EventType, void*)
{
    const auto* d = static_cast<const crosspad::MotionGestureData*>(data);
    s_gesture.store(static_cast<int>(d->gesture));
    if (d->gesture != crosspad::MotionGesture::SingleTap &&
        d->gesture != crosspad::MotionGesture::DoubleTap) return;
    s_taps.fetch_add(1);
    auto& vib = crosspad::getVibration();
    if (vib.isAvailable()) {
        vib.vibrate(d->gesture == crosspad::MotionGesture::SingleTap
                        ? crosspad::VibrationPreset::ImuTap
                        : crosspad::VibrationPreset::ImuDoubleTap);
    }
}
// [/docs:gesture]

// [docs:poll]
void tick(lv_timer_t*)
{
    const int taps = s_taps.load();
    const int gesture = s_gesture.load();
    if (taps != s_shownTaps || gesture != s_shownGesture) {
        s_shownTaps = taps;
        s_shownGesture = gesture;
        lv_label_set_text_fmt(s_info, "Taps: %d   last: %s", taps, gesture_name(gesture));
    }

    auto& imu = crosspad::getImu();
    const bool sensor = imu.isAvailable();   // the simulator has no motion sensor
    if (s_shownSensor != (sensor ? 1 : 0)) {   // first tick, or the sensor came or went
        s_shownSensor = sensor ? 1 : 0;
        if (sensor) {
            s_lastCc = -1;   // send the current tilt as soon as the sensor is back
            lv_label_set_text_fmt(s_cc, "Tilt to send CC %u",
                                  static_cast<unsigned>(example_motion::kTiltCc));
        } else {
            lv_label_set_text(s_cc, "No motion sensor here.");
        }
    }
    if (!sensor) return;   // nothing to read, nothing to send
    crosspad::ImuData d;
    if (!imu.getData(d)) return;
    lv_obj_set_pos(s_dot, dot_pos(d.roll), dot_pos(d.pitch));

    const uint8_t cc = example_motion::tiltToCc(d.roll);
    if (!example_motion::worthSending(s_lastCc, cc)) return;
    crosspad::getMidiOutput().sendControlChange(example_motion::kTiltCc, cc, 0);   // channel 1
    s_lastCc = cc;
    lv_label_set_text_fmt(s_cc, "CC %u = %d, channel 1",
                          static_cast<unsigned>(example_motion::kTiltCc), s_lastCc);
}
// [/docs:poll]

} // namespace

lv_obj_t* Motion_create(lv_obj_t* parent, App*)
{
    // Every static starts clean on each visit.
    s_taps.store(0);
    s_gesture.store(-1);
    s_sub = nullptr;
    s_timer = nullptr;
    s_dot = s_cc = s_info = nullptr;
    s_lastCc = -1;
    s_shownTaps = -1;
    s_shownGesture = -2;
    s_shownSensor = -1;   // the first tick paints the sensor line

    lv_obj_t* root = lv_obj_create(parent);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* box = lv_obj_create(root);   // a spirit level: the dot moves with the tilt
    lv_obj_remove_style_all(box);
    lv_obj_remove_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(box, kBox, kBox);
    lv_obj_set_style_border_width(box, 1, 0);
    lv_obj_set_style_border_color(box, lv_color_hex(0x7B828B), 0);
    lv_obj_align(box, LV_ALIGN_LEFT_MID, 0, 0);
    s_dot = lv_obj_create(box);
    lv_obj_remove_style_all(s_dot);
    lv_obj_set_size(s_dot, kDot, kDot);
    lv_obj_set_style_radius(s_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(s_dot, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(s_dot, lv_color_hex(0xFF5C5C), 0);
    lv_obj_set_pos(s_dot, kHalf, kHalf);

    s_cc = lv_label_create(root);
    lv_obj_align(s_cc, LV_ALIGN_TOP_RIGHT, 0, 20);
    s_info = lv_label_create(root);
    lv_obj_align(s_info, LV_ALIGN_TOP_RIGHT, 0, 50);

    // [docs:subscribe]
    s_sub = crosspad::getEventBus().subscribe(crosspad::EventType::MotionGesture,
                                              on_gesture, nullptr);
    // [/docs:subscribe]
    s_timer = lv_timer_create(tick, 50, nullptr);
    return root;
}

// [docs:destroy]
void Motion_destroy(lv_obj_t* root)
{
    // root is null when the app's screen was already wiped: free what is ours, delete nothing.
    if (s_timer) { lv_timer_delete(s_timer); s_timer = nullptr; }
    if (s_sub) { crosspad::getEventBus().unsubscribe(s_sub); s_sub = nullptr; }
    s_dot = s_cc = s_info = nullptr;
    if (root) lv_obj_delete_async(root);
}
// [/docs:destroy]

// No pad logic: the pads keep playing the kit; this app only reads the motion sensor.
REGISTER_APP(Motion, nullptr, "example-motion.png",
             Motion_create, Motion_destroy,
             nullptr, nullptr, nullptr, 0)
