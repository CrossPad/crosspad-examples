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
#include "example-hello-screen/HelloModel.hpp"

/*
 * Hello Screen -- the smallest complete CrossPad app.
 * Recipe: https://crosspad.app/docs/apps/cookbook/hello-screen/
 */
namespace {

// [docs:state]
lv_obj_t* s_root = nullptr;      // the app's screen
lv_obj_t* s_count = nullptr;     // "Pressed N times"
lv_obj_t* s_details = nullptr;   // a panel the power button closes first
lv_group_t* s_group = nullptr;   // what the encoder walks
int s_presses = 0;
// [/docs:state]

// [docs:press]
void on_press(lv_event_t*)
{
    ++s_presses;
    lv_label_set_text(s_count, example_hello::pressedLabel(s_presses).c_str());
}
// [/docs:press]

// [docs:modal]
// A hidden panel does not stop the encoder: it still clicks whatever is
// focused behind it. A disabled button is skipped by the encoder and ignores
// its click, so the screen is disabled while the panel is up.
void show_details(bool show)
{
    lv_obj_set_hidden(s_details, !show);
    for (uint32_t i = 0; i < lv_obj_get_child_count(s_root); ++i)
        lv_obj_set_state(lv_obj_get_child(s_root, i), LV_STATE_DISABLED, show);
}
// [/docs:modal]

void on_details(lv_event_t*) { show_details(true); }

// [docs:close]
void on_close(lv_event_t* e)
{
    // Never destroyApp() from inside an event: it deletes the very button
    // this event is running on. destroyAppAsync() waits for the event to end.
    auto* app = static_cast<App*>(lv_event_get_user_data(e));
    app->destroyAppAsync();
}
// [/docs:close]

// [docs:back]
// The power button's click. true = handled here (the panel closes);
// false = let the framework close the app.
bool on_back(lv_obj_t*)
{
    if (s_details && !lv_obj_has_flag(s_details, LV_OBJ_FLAG_HIDDEN)) {
        show_details(false);
        return true;
    }
    return false;
}
// [/docs:back]

lv_obj_t* add_button(lv_obj_t* parent, const char* text, lv_event_cb_t cb, void* user)
{
    lv_obj_t* b = lv_button_create(parent);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, user);
    lv_obj_t* l = lv_label_create(b);
    lv_label_set_text(l, text);
    lv_obj_center(l);
    if (s_group) lv_group_add_obj(s_group, b);
    return b;
}

} // namespace

// [docs:create]
lv_obj_t* HelloScreen_create(lv_obj_t* parent, App* app)
{
    s_group = nullptr;
    s_root = nullptr;
    s_count = nullptr;
    s_details = nullptr;
    s_presses = 0;

    // The encoder drives whichever group is on it. This app's buttons go in
    // its own group; the framework gives the encoder back when the app closes.
    if (lv_indev_t* enc = crosspad_gui::getGuiPlatform().getNavigationEncoder()) {
        s_group = lv_group_create();
        lv_indev_set_group(enc, s_group);
    }

    lv_obj_t* root = lv_obj_create(parent);
    s_root = root;
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(root, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(root);
    lv_label_set_text(title, "Hello, CrossPad");

    add_button(root, "Press me", on_press, nullptr);
    s_count = lv_label_create(root);
    lv_label_set_text(s_count, example_hello::pressedLabel(0).c_str());
    add_button(root, "Details", on_details, nullptr);
    add_button(root, LV_SYMBOL_CLOSE " Close", on_close, app);

    s_details = lv_obj_create(parent);     // on top of root, hidden until asked
    lv_obj_set_size(s_details, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(s_details, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(s_details, 16, 0);
    lv_obj_t* note = lv_label_create(s_details);
    lv_label_set_text(note, "A panel inside the app.\n\nClick the power button:\nthis panel closes, the app stays.");
    lv_obj_set_hidden(s_details, true);

    app->setOnBack(on_back);
    return root;
}
// [/docs:create]

// [docs:destroy]
void HelloScreen_destroy(lv_obj_t* root)
{
    // root is null when the app's screen was already wiped: free what is ours, delete nothing.
    if (s_group) {
        lv_group_delete_deferred(s_group);   // the encoder may still be mid-read
        s_group = nullptr;
    }
    if (root && s_details) lv_obj_delete_async(s_details);
    s_details = nullptr;
    s_root = nullptr;
    s_count = nullptr;
    if (root) lv_obj_delete_async(root);
}
// [/docs:destroy]

// [docs:register]
REGISTER_APP(HelloScreen, nullptr, "example-hello-screen.png",
             HelloScreen_create, HelloScreen_destroy,
             nullptr, nullptr, nullptr, 0)
// [/docs:register]
