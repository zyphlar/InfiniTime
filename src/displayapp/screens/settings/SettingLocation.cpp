#include "displayapp/screens/settings/SettingLocation.h"
#include <lvgl/lvgl.h>
#include <nrf_log.h>
#include "displayapp/DisplayApp.h"
#include "displayapp/screens/Symbols.h"
#include "components/settings/Settings.h"
#include "displayapp/InfiniTimeTheme.h"

using namespace Pinetime::Applications::Screens;

namespace {
  constexpr int16_t POS_Y_TEXT = 25;

  void ValueChangedHandler(void* userData) {
    auto* screen = static_cast<SettingLocation*>(userData);
    screen->UpdateScreen();
  }
}

SettingLocation::SettingLocation(Pinetime::Controllers::Settings& settingsController)
  : settingsController {settingsController} {

  lv_obj_t* title = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(title, "Set location\n(lat/long/tz)");
  lv_label_set_align(title, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(title, lv_scr_act(), LV_ALIGN_IN_TOP_MID, 15, 15);

  lv_obj_t* icon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(icon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_ORANGE);
  lv_label_set_text_static(icon, Symbols::map);
  lv_label_set_align(icon, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(icon, title, LV_ALIGN_OUT_LEFT_MID, -10, 0);

  Controllers::Settings::Location loc = settingsController.GetLocation();

  latCounter.Create();
  latCounter.SetWidth(80);
  latCounter.SetValue(loc.latitude);
  lv_obj_align(latCounter.GetObject(), nullptr, LV_ALIGN_CENTER, -90, POS_Y_TEXT);
  latCounter.SetValueChangedEventCallback(this, ValueChangedHandler);

  longCounter.Create();
  longCounter.SetWidth(110);
  longCounter.SetValue(loc.longitude);
  lv_obj_align(longCounter.GetObject(), nullptr, LV_ALIGN_CENTER, -5, POS_Y_TEXT);
  longCounter.SetValueChangedEventCallback(this, ValueChangedHandler);

  tzCounter.Create();
  tzCounter.SetWidth(60);
  tzCounter.SetValue(loc.tzOffset);
  lv_obj_align(tzCounter.GetObject(), nullptr, LV_ALIGN_CENTER, 75, POS_Y_TEXT);
  tzCounter.SetValueChangedEventCallback(this, ValueChangedHandler);

  UpdateScreen();
}

SettingLocation::~SettingLocation() {
  lv_obj_clean(lv_scr_act());
  settingsController.SaveSettings();
}

void SettingLocation::UpdateScreen() {
  Controllers::Settings::Location loc = {
    latitude: (int16_t)latCounter.GetValue(),
    longitude: (int16_t)longCounter.GetValue(),
    tzOffset: (int8_t)tzCounter.GetValue(),
  };
  settingsController.SetLocation(loc);
}