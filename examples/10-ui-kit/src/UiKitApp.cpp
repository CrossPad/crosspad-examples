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
#include "example-ui-kit/UiModel.hpp"
#include "crosspad-gui/design/cp_design.h"
#include "crosspad-gui/theme/crosspad_theme.h"
#include "crosspad-gui/components/ui_toast.h"
#include "crosspad-gui/components/ui_modal.h"
#include "crosspad-gui/components/power_gesture.h"
#include "crosspad/event/IEventBus.hpp"
#include "crosspad/event/EventData.hpp"
#include <atomic>

/*
 * UI Kit -- CrossPad's look, toasts, a confirm dialog, an overlay the power
 * button closes, and pad hits from another task shown safely.
 * Recipe: https://crosspad.app/docs/apps/cookbook/ui-kit/
 */
namespace {

// [docs:state]
lv_obj_t* s_count = nullptr;                    // "Pads hit: N"
lv_obj_t* s_row = nullptr;                      // the row of buttons
lv_obj_t* s_panel = nullptr;                    // the overlay, hidden until asked
lv_group_t* s_group = nullptr;                  // what the encoder walks
lv_obj_t* s_dialog = nullptr;                   // the confirm dialog while it is open
crosspad::SubscriptionHandle s_sub = nullptr;   // our SamplePlayback subscription
uint32_t s_backToken = 0;                       // the panel's place on the back stack, 0 = none
std::atomic<bool> s_alive{false};               // false once the app is closing
int s_pads = 0;                                 // pad hits counted since the last reset
// [/docs:state]

// [docs:helpers]
void show_count() { lv_label_set_text_fmt(s_count, "Pads hit: %d", s_pads); }

// A hidden panel does not stop the encoder: it still clicks whatever is
// focused behind it. A disabled button is skipped and ignores its click, so
// the buttons are disabled while the panel is up.
void show_panel(bool show)
{
    lv_obj_set_hidden(s_panel, !show);
    for (uint32_t i = 0; i < lv_obj_get_child_count(s_row); ++i)
        lv_obj_set_state(lv_obj_get_child(s_row, i), LV_STATE_DISABLED, show);
}
// [/docs:helpers]

// [docs:post]
// On the LVGL task, when the queue reaches it. s_alive is false once the app
// is closing, so a hit that was queued before the close does nothing.
void show_pad(void* p)
{
    if (!s_alive.load()) return;
    ui_toast_show(example_ui::padToast(example_ui::unpackPad(p)).c_str(), 800, UI_TOAST_INFO);
    ++s_pads;
    show_count();
}

// On the event task: no LVGL here. Hand the pad over to the LVGL task.
// These are the pads the Sampler plays (from a finger or a running sequence);
// without the Sampler as the base pad logic no event arrives. PadPressed is not
// used: it is posted only when no pad logic at all is active.
void on_pad(const void* data, crosspad::EventType, void*)
{
    const auto* d = static_cast<const crosspad::SamplePlaybackData*>(data);
    if (!d->isOn || d->note >= 16 || d->source != crosspad::EventSource::App) return;
    crosspad_gui::getGuiPlatform().postToGui(show_pad, example_ui::packPad(d->note));
}
// [/docs:post]

void on_toast(lv_event_t*) { ui_toast_show("Saved", 1500, UI_TOAST_SUCCESS); }

// [docs:modal]
void on_confirm(bool yes, void*)
{
    s_dialog = nullptr;   // the last moment the dialog is alive
    if (!yes || !s_alive.load()) return;
    s_pads = 0;
    show_count();
}

void on_ask(lv_event_t*)
{
    // The dialog takes the encoder and gives it back to s_group when it closes.
    s_dialog = ui_modal_confirm("Reset counter?", "The pad counter goes back to zero.",
                                "Reset", "Cancel", on_confirm, nullptr, s_group);
}
// [/docs:modal]

// [docs:back-stack]
// An overlay registers on the back stack while it is up: the power button's
// click asks the newest overlay first, and only when none takes it does the
// click reach the app.
bool on_panel_back(void*)
{
    if (!s_panel) return false;
    show_panel(false);
    crosspad_gui::ui_back_remove(s_backToken);
    s_backToken = 0;
    return true;   // used up: the app stays open
}

void on_panel(lv_event_t*)
{
    if (!s_backToken) s_backToken = crosspad_gui::ui_back_push(on_panel_back, nullptr);
    if (!s_backToken) return;   // the back stack is full: no way to close it, so do not open
    show_panel(true);
}
// [/docs:back-stack]

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

lv_obj_t* UiKit_create(lv_obj_t* parent, App*)
{
    // Every static starts clean on each visit.
    s_count = s_row = s_panel = nullptr;
    s_group = nullptr;
    s_dialog = nullptr;
    s_sub = nullptr;
    s_backToken = 0;
    s_pads = 0;
    s_alive.store(true);

    if (lv_indev_t* enc = crosspad_gui::getGuiPlatform().getNavigationEncoder()) {
        s_group = lv_group_create();
        lv_indev_set_group(enc, s_group);
    }

    lv_obj_t* root = lv_obj_create(parent);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(root, 8, 0);

    // [docs:look]
    // The pixel face is ASCII only: text that comes from the card (kit and
    // file names) stays on lv_font_montserrat_*.
    lv_obj_t* title = lv_label_create(root);
    lv_obj_set_style_text_font(title, CP_FONT_PIXEL, 0);
    lv_obj_set_style_text_color(title, CROSSPAD_THEME_PRIMARY, 0);   // the theme's colour
    lv_label_set_text(title, "UI KIT");

    lv_obj_t* rule = lv_obj_create(root);
    lv_obj_remove_style_all(rule);
    lv_obj_set_size(rule, LV_PCT(100), 1);
    lv_obj_set_style_bg_opa(rule, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(rule, CP_COL_HAIRLINE, 0);

    s_count = lv_label_create(root);
    lv_obj_set_style_text_color(s_count, CP_COL_NORMAL, 0);
    // [/docs:look]
    show_count();

    s_row = lv_obj_create(root);
    lv_obj_remove_style_all(s_row);
    lv_obj_set_size(s_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(s_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(s_row, 8, 0);
    add_button(s_row, "Toast", on_toast);
    add_button(s_row, "Ask", on_ask);
    add_button(s_row, "Panel", on_panel);

    s_panel = lv_obj_create(parent);   // over the whole app, hidden until asked
    lv_obj_set_size(s_panel, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(s_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(s_panel, 16, 0);
    lv_obj_t* note = lv_label_create(s_panel);
    lv_label_set_text(note, "An overlay inside the app.\n\nThe power button closes it first.");
    lv_obj_set_hidden(s_panel, true);

    // No pad logic of our own: the Sampler keeps playing the pads and posts each hit.
    s_sub = crosspad::getEventBus().subscribe(crosspad::EventType::SamplePlayback,
                                              on_pad, nullptr);
    return root;
}

// [docs:destroy]
void UiKit_destroy(lv_obj_t* root)
{
    s_alive.store(false);   // a show_pad() still queued will see this
    if (s_sub) { crosspad::getEventBus().unsubscribe(s_sub); s_sub = nullptr; }
    // An open dialog hands the encoder back to s_group: close it before s_group goes.
    if (s_dialog) ui_modal_confirm_dismiss(s_dialog, false);
    crosspad_gui::ui_back_remove(s_backToken);   // a panel left open leaves no handler behind
    s_backToken = 0;
    ui_toast_dismiss();
    if (s_group) { lv_group_delete_deferred(s_group); s_group = nullptr; }
    if (s_panel) lv_obj_delete_async(s_panel);
    s_panel = nullptr;
    s_row = nullptr;
    s_count = nullptr;
    lv_obj_delete_async(root);
}
// [/docs:destroy]

REGISTER_APP(UiKit, nullptr, "example-ui-kit.png",
             UiKit_create, UiKit_destroy,
             nullptr, nullptr, nullptr, 0)
