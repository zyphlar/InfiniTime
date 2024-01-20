#include "displayapp/screens/WatchFaceDigital.h"

#include <lvgl/lvgl.h>
#include <cstdio>
#include "displayapp/screens/NotificationIcon.h"
#include "displayapp/screens/Symbols.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "components/heartrate/HeartRateController.h"
#include "components/motion/MotionController.h"
#include "components/settings/Settings.h"

using namespace Pinetime::Applications::Screens;

WatchFaceDigital::WatchFaceDigital(Controllers::DateTime& dateTimeController,
                                   const Controllers::Battery& batteryController,
                                   const Controllers::Ble& bleController,
                                   Controllers::NotificationManager& notificationManager,
                                   Controllers::Settings& settingsController,
                                   Controllers::HeartRateController& heartRateController,
                                   Controllers::MotionController& motionController)
  : currentDateTime {{}},
    dateTimeController {dateTimeController},
    notificationManager {notificationManager},
    settingsController {settingsController},
    heartRateController {heartRateController},
    motionController {motionController},
    statusIcons(batteryController, bleController) {

  sHour = 99;
  sMinute = 99;
  statusIcons.Create();

  lv_obj_t* bg = lv_img_create(lv_scr_act(), nullptr);
  lv_img_set_src(bg, "F:/images/aor.bin");
  lv_img_set_auto_size(bg, false);
  lv_obj_set_size(bg, 240, 240);
  // lv_img_set_src(bg, image.fileName);
  lv_img_set_offset_x(bg, 0);
  lv_img_set_offset_y(bg, 0);
  // lv_obj_set_style_local_image_recolor_opa(bg, LV_IMG_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_COVER);
  // lv_obj_set_style_local_image_recolor(bg, LV_IMG_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_CYAN);
  lv_obj_align(bg, nullptr, LV_ALIGN_CENTER, 0, 0);

  notificationIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(notificationIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_LIME);
  lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(false));
  lv_obj_align(notificationIcon, nullptr, LV_ALIGN_IN_TOP_LEFT, 0, 0);

  label_date = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_date, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x999999));

  label_time = lv_label_create(lv_scr_act(), nullptr);

  label_time_ampm = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(label_time_ampm, "");
  lv_obj_align(label_time_ampm, lv_scr_act(), LV_ALIGN_IN_RIGHT_MID, -30, -55);

  heartbeatIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(heartbeatIcon, Symbols::heartBeat);
  lv_obj_set_style_local_text_color(heartbeatIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xCE1B1B));
  lv_obj_align(heartbeatIcon, lv_scr_act(), LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);

  heartbeatValue = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(heartbeatValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xCE1B1B));
  lv_label_set_text_static(heartbeatValue, "");
  lv_obj_align(heartbeatValue, heartbeatIcon, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

  stepValue = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(stepValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x00FFE7));
  lv_label_set_text_static(stepValue, "0");
  lv_obj_align(stepValue, lv_scr_act(), LV_ALIGN_IN_BOTTOM_RIGHT, 0, 0);

  stepIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(stepIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x00FFE7));
  lv_label_set_text_static(stepIcon, Symbols::shoe);
  lv_obj_align(stepIcon, stepValue, LV_ALIGN_OUT_LEFT_MID, -5, 0);

  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
  Refresh();
}

WatchFaceDigital::~WatchFaceDigital() {
  lv_task_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
}

void WatchFaceDigital::Refresh() {
  statusIcons.Update();

  notificationState = notificationManager.AreNewNotificationsAvailable();
  if (notificationState.IsUpdated()) {
    lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(notificationState.Get()));
  }

  currentDateTime = dateTimeController.CurrentDateTime();

  if (currentDateTime.IsUpdated()) {

    uint8_t hour = dateTimeController.Hours();
    uint8_t minute = dateTimeController.Minutes();

    if (sHour != hour || sMinute != minute || forceRefresh == true) {
      forceRefresh = false;
      sHour = hour;
      sMinute = minute;

      if (settingsController.GetClockType() == Controllers::Settings::ClockType::Fuzzy) {
        printTimeWords(static_cast<int>(hour), static_cast<int>(minute));
        lv_label_set_text(label_time_ampm, "");
        lv_obj_align(label_time, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, -10);
        lv_obj_set_style_local_text_font(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_42);
        lv_label_set_recolor(label_time, true);
        lv_obj_align(label_date, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, 60);
      } else if (settingsController.GetClockType() == Controllers::Settings::ClockType::H12) {
        char ampmChar[3] = "AM";
        if (hour == 0) {
          hour = 12;
        } else if (hour == 12) {
          ampmChar[0] = 'P';
        } else if (hour > 12) {
          hour = hour - 12;
          ampmChar[0] = 'P';
        }
        lv_label_set_text(label_time_ampm, ampmChar);
        lv_label_set_text_fmt(label_time, "%2d:%02d", hour, minute);
        lv_obj_align(label_time, lv_scr_act(), LV_ALIGN_IN_RIGHT_MID, 0, 0);
        lv_obj_set_style_local_text_font(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_extrabold_compressed);
        lv_obj_align(label_date, lv_scr_act(), LV_ALIGN_CENTER, 0, 60);
      } else {
        lv_label_set_text_fmt(label_time, "%02d:%02d", hour, minute);
        lv_obj_align(label_time, lv_scr_act(), LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_local_text_font(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_extrabold_compressed);
        lv_obj_align(label_date, lv_scr_act(), LV_ALIGN_CENTER, 0, 60);
      }
      lv_obj_realign(label_time);
      lv_obj_realign(label_date);
    }

    currentDate = std::chrono::time_point_cast<days>(currentDateTime.Get());
    if (currentDate.IsUpdated()) {
      uint16_t year = dateTimeController.Year();
      uint8_t day = dateTimeController.Day();
      if (settingsController.GetClockType() == Controllers::Settings::ClockType::H24) {
        lv_label_set_text_fmt(label_date,
                              "%s %d %s %d",
                              dateTimeController.DayOfWeekShortToString(),
                              day,
                              dateTimeController.MonthShortToString(),
                              year);
      } else {
        lv_label_set_text_fmt(label_date,
                              "%s %s %d %d",
                              dateTimeController.DayOfWeekShortToString(),
                              dateTimeController.MonthShortToString(),
                              day,
                              year);
      }
      lv_obj_realign(label_date);
    }
  }

  heartbeat = heartRateController.HeartRate();
  heartbeatRunning = heartRateController.State() != Controllers::HeartRateController::States::Stopped;
  if (heartbeat.IsUpdated() || heartbeatRunning.IsUpdated()) {
    if (heartbeatRunning.Get()) {
      lv_obj_set_style_local_text_color(heartbeatIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xCE1B1B));
      lv_label_set_text_fmt(heartbeatValue, "%d", heartbeat.Get());
    } else {
      lv_obj_set_style_local_text_color(heartbeatIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x1B1B1B));
      lv_label_set_text_static(heartbeatValue, "");
    }

    lv_obj_realign(heartbeatIcon);
    lv_obj_realign(heartbeatValue);
  }

  stepCount = motionController.NbSteps();
  if (stepCount.IsUpdated()) {
    lv_label_set_text_fmt(stepValue, "%lu", stepCount.Get());
    lv_obj_realign(stepValue);
    lv_obj_realign(stepIcon);
  }
}

bool WatchFaceDigital::OnTouchEvent(Pinetime::Applications::TouchEvents event) {
  if ((event == Pinetime::Applications::TouchEvents::LongTap)) {
    Pinetime::Controllers::Settings::ClockType clockType = settingsController.GetClockType();
    if (clockType == Pinetime::Controllers::Settings::ClockType::Fuzzy) {
      settingsController.SetClockType(Pinetime::Controllers::Settings::ClockType::H12);
    } else {
      settingsController.SetClockType(Pinetime::Controllers::Settings::ClockType::Fuzzy);
    }
    settingsController.SaveSettings();
    forceRefresh=true;
    WatchFaceDigital::Refresh();
    return true;
  }
  return false;
}


char const* twelve = "twelve";

void WatchFaceDigital::printTimeWords(int h, int m) {
  const char* mod;
  char const* accent = "#808080";
  char const* color = "#ffffff";
  char const* oclock = "o' clock";
  char const* past = "past";
  char const* to = "to";
  char const* hash = "#";
  char const* nearly = "nearly";
  char const* fmt = "%s %s%s\n%s %s%s %s %s%s";

  if (m  <= 30) {
    mod = mods[m / 5];
  } else {
    mod = mods[(60-m) / 5];
  }
  h  = (h % 12);

  if (m >= 56) {
    lv_label_set_text_fmt(label_time, "%s %s %s%s\n%s %s%s", color, nearly, nums[(h+1) % 12], hash, accent, oclock, hash);
  }
  else if (m == 0 || m <= 4) {
    lv_label_set_text_fmt(label_time, "%s %s%s\n%s %s%s", color, nums[h], hash, accent, oclock, hash);
  }

  else if (m <= 32) {
    lv_label_set_text_fmt(label_time, fmt, color, mod, hash, accent, past, hash, color, nums[h], hash);
  }

  else if (m > 32) {
    lv_label_set_text_fmt(label_time, fmt, color, mod, hash, accent, to, hash, color, nums[(h+1) % 12], hash);
  }

}