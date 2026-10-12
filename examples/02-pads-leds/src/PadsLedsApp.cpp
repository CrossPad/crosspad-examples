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
#include "example-pads-leds/PadsLedsPadLogic.hpp"
#include "example-pads-leds/PadsModel.hpp"
#include "crosspad/pad/PadManager.hpp"
#include "crosspad/pad/PadAnimator.hpp"
#include <memory>

/*
 * Pads and LEDs -- the pads light in their own colour, harder hits brighter,
 * and the screen mirrors the 4x4 grid (pad 1 bottom-left).
 * Recipe: https://crosspad.app/docs/apps/cookbook/pads-leds/
 */
namespace {

// [docs:state]
std::shared_ptr<PadsLedsPadLogic> s_logic;   // kept for the life of the program (see create)
lv_obj_t* s_cells[16] = {};      // one square per pad
lv_obj_t* s_status = nullptr;    // "Pad N  level L"
lv_timer_t* s_timer = nullptr;   // redraws the cells from the recorded levels
lv_group_t* s_group = nullptr;   // what the encoder walks
lv_obj_t* s_rainbow_label = nullptr;   // "Rainbow" / "Stop"
bool s_rainbow = false;          // this app started the PadAnimator frame
// [/docs:state]

// [docs:timer]
// Runs on the LVGL task every 33 ms: reads what the pad task recorded, draws it.
void tick(lv_timer_t*)
{
    for (uint8_t p = 0; p < 16; ++p) {
        const uint8_t lvl = s_logic->level(p);
        const auto c = example_pads::padColor(p, lvl ? lvl : example_pads::kRestLevel);
        lv_obj_set_style_bg_color(s_cells[p], lv_color_make(c.r, c.g, c.b), 0);
    }
    const int last = s_logic->lastPad();
    if (last >= 0) {
        lv_label_set_text_fmt(s_status, "Pad %d  level %u", last + 1,
                              static_cast<unsigned>(s_logic->level(static_cast<uint8_t>(last))));
    }
}
// [/docs:timer]

// [docs:rainbow]
// PadAnimator holds a whole frame until stop(): while it shows, pad presses
// do not repaint the LEDs. Good for a moment, not for playing.
// isActive() says an animation is showing, not whose: if the board starts its
// own (an update's progress bar) while the rainbow is up, Stop ends that one
// too. Fine for a demo; an app that must not interfere checks before it
// starts and leaves the animator alone after.
void on_rainbow(lv_event_t*)
{
    auto& anim = crosspad::getPadAnimator();
    if (s_rainbow && anim.isActive()) {
        anim.stop();
        s_rainbow = false;
        lv_label_set_text(s_rainbow_label, "Rainbow");
        return;
    }
    s_rainbow = false;
    lv_label_set_text(s_rainbow_label, "Rainbow");
    if (anim.isActive()) return;
    crosspad::RgbColor frame[crosspad::PadAnimator::kPadCount];
    for (uint8_t p = 0; p < 16; ++p) {
        const auto c = example_pads::wheel(p);
        frame[p] = crosspad::RgbColor(c.r, c.g, c.b);
    }
    anim.showFrame(frame);
    s_rainbow = true;
    lv_label_set_text(s_rainbow_label, "Stop");
}
// [/docs:rainbow]

} // namespace

lv_obj_t* PadsLeds_create(lv_obj_t* parent, App*)
{
    // Every static starts clean on each visit -- except s_logic, on purpose.
    for (auto& c : s_cells) c = nullptr;
    s_status = nullptr;
    s_timer = nullptr;
    s_group = nullptr;
    s_rainbow_label = nullptr;
    s_rainbow = false;

    // [docs:register-logic]
    // Registered once and kept: REGISTER_APP_PL below makes the framework
    // push it when the app opens and pop it when the app closes.
    if (!s_logic) {
        s_logic = std::make_shared<PadsLedsPadLogic>();
        crosspad::getPadManager().registerPadLogic("PadsLeds", s_logic);
    }
    // [/docs:register-logic]

    if (lv_indev_t* enc = crosspad_gui::getGuiPlatform().getNavigationEncoder()) {
        s_group = lv_group_create();
        lv_indev_set_group(enc, s_group);
    }

    lv_obj_t* root = lv_obj_create(parent);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    // [docs:grid]
    // The screen is a picture of the pads: row 0 on screen is pads 13-16.
    const int32_t cell = 36, gap = 4;
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            const int pad = (3 - row) * 4 + col;
            lv_obj_t* c = lv_obj_create(root);
            lv_obj_remove_style_all(c);
            lv_obj_set_size(c, cell, cell);
            lv_obj_set_pos(c, col * (cell + gap), row * (cell + gap));
            lv_obj_set_style_radius(c, 4, 0);
            lv_obj_set_style_bg_opa(c, LV_OPA_COVER, 0);
            s_cells[pad] = c;
        }
    }
    // [/docs:grid]

    s_status = lv_label_create(root);
    lv_obj_align(s_status, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_label_set_text(s_status, "Hit a pad");

    lv_obj_t* btn = lv_button_create(root);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_add_event_cb(btn, on_rainbow, LV_EVENT_CLICKED, nullptr);
    s_rainbow_label = lv_label_create(btn);
    lv_label_set_text(s_rainbow_label, "Rainbow");
    if (s_group) lv_group_add_obj(s_group, btn);

    s_timer = lv_timer_create(tick, 33, nullptr);
    return root;
}

// [docs:destroy]
void PadsLeds_destroy(lv_obj_t* root)
{
    // root is null when the app's screen was already wiped: free what is ours, delete nothing.
    if (s_rainbow && crosspad::getPadAnimator().isActive()) crosspad::getPadAnimator().stop();
    s_rainbow = false;
    s_rainbow_label = nullptr;
    lv_timer_delete(s_timer);
    s_timer = nullptr;
    if (s_group) { lv_group_delete_deferred(s_group); s_group = nullptr; }
    for (auto& c : s_cells) c = nullptr;
    s_status = nullptr;
    if (root) lv_obj_delete_async(root);
}
// [/docs:destroy]

// [docs:register]
REGISTER_APP_PL(PadsLeds, nullptr, "example-pads-leds.png",
                PadsLeds_create, PadsLeds_destroy,
                nullptr, nullptr, nullptr, 0, "PadsLeds")
// [/docs:register]
