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
#include "example-play-sample/PlaySamplePadLogic.hpp"
#include "crosspad/pad/PadManager.hpp"
#include "crosspad/platform/PlatformServices.hpp"
#include "crosspad/kit/IKitManager.hpp"
#include <memory>

/*
 * Play a Sample -- the kit as the Sampler plays it, or pad 1's sample pitched
 * across all sixteen pads.
 * Recipe: https://crosspad.app/docs/apps/cookbook/play-sample/
 */
namespace {

// [docs:state]
std::shared_ptr<PlaySamplePadLogic> s_logic;   // kept for the life of the program (see create)
lv_obj_t* s_mode = nullptr;      // "Mode: Kit ..." / "Mode: Pitched ..."
lv_group_t* s_group = nullptr;   // what the encoder walks
// [/docs:state]

// [docs:kit]
const char* current_kit_name()
{
    crosspad::IKitManager* kits = crosspad::getKitManager();   // null on a platform without kits
    crosspad::KitInfo* kit = kits ? kits->getCurrentKit() : nullptr;
    return kit ? kit->name.c_str() : nullptr;
}
// [/docs:kit]

void show_mode()
{
    lv_label_set_text(s_mode, s_logic->mode() == example_play::Mode::Kit
                                  ? "Mode: Kit (every pad its own sound)"
                                  : "Mode: Pitched (pad 1's sound on every pad)");
}

void on_toggle(lv_event_t*)
{
    using example_play::Mode;
    s_logic->setMode(s_logic->mode() == Mode::Kit ? Mode::Pitched : Mode::Kit);
    show_mode();
}

} // namespace

lv_obj_t* PlaySample_create(lv_obj_t* parent, App*)
{
    // Every static starts clean on each visit -- except s_logic, on purpose.
    s_mode = nullptr;
    s_group = nullptr;

    // Registered once and kept: REGISTER_APP_PL below makes the framework
    // push it when the app opens and pop it when the app closes.
    if (!s_logic) {
        s_logic = std::make_shared<PlaySamplePadLogic>();
        crosspad::getPadManager().registerPadLogic("PlaySample", s_logic);
    }
    s_logic->setMode(example_play::Mode::Kit);

    if (lv_indev_t* enc = crosspad_gui::getGuiPlatform().getNavigationEncoder()) {
        s_group = lv_group_create();
        lv_indev_set_group(enc, s_group);
    }

    lv_obj_t* root = lv_obj_create(parent);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(root, 10, 0);

    lv_obj_t* kit = lv_label_create(root);
    lv_obj_set_width(kit, LV_PCT(100));
    const char* name = current_kit_name();
    if (name) lv_label_set_text_fmt(kit, "Kit: %s", name);
    else      lv_label_set_text(kit, "No kit loaded. MODE, then pad 13 (Kits).");

    s_mode = lv_label_create(root);
    lv_obj_set_width(s_mode, LV_PCT(100));   // wrap inside the screen instead of scrolling it sideways
    show_mode();

    lv_obj_t* btn = lv_button_create(root);
    lv_obj_add_event_cb(btn, on_toggle, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* l = lv_label_create(btn);
    lv_label_set_text(l, "Switch mode");
    if (s_group) lv_group_add_obj(s_group, btn);
    return root;
}

void PlaySample_destroy(lv_obj_t* root)
{
    if (s_group) { lv_group_delete_deferred(s_group); s_group = nullptr; }
    s_mode = nullptr;
    lv_obj_delete_async(root);
}

REGISTER_APP_PL(PlaySample, nullptr, "example-play-sample.png",
                PlaySample_create, PlaySample_destroy,
                nullptr, nullptr, nullptr, 0, "PlaySample")
