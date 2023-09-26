#include "displayapp/screens/WatchFaceFuzzy.h"

#include <lvgl/lvgl.h>
#include <cstdio>
#include <lvgl/src/lv_core/lv_obj_style_dec.h>
#include <lvgl/src/lv_font/lv_font.h>
#include <lvgl/src/lv_misc/lv_area.h>
#include "displayapp/screens/NotificationIcon.h"
#include "displayapp/screens/Symbols.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "components/heartrate/HeartRateController.h"
#include "components/motion/MotionController.h"

using namespace Pinetime::Applications::Screens;

WatchFaceFuzzy::WatchFaceFuzzy(Controllers::DateTime& dateTimeController,
                               const Controllers::Battery& batteryController,
                                   const Controllers::Ble& bleController,
                                   Controllers::NotificationManager& notificationManager,
                                   Controllers::HeartRateController& heartRateController,
                                   Controllers::MotionController& motionController)
  : currentDateTime {{}},
    dateTimeController {dateTimeController},
    notificationManager {notificationManager},
    heartRateController {heartRateController},
    motionController {motionController},
    statusIcons(batteryController, bleController) {

  statusIcons.Create();

  notificationIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(notificationIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_LIME);
  lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(false));
  lv_obj_align(notificationIcon, nullptr, LV_ALIGN_IN_TOP_LEFT, 0, 0);

  label_time = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &vulf_mono_italic);
  lv_obj_align(label_time, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, -20);
  lv_label_set_recolor(label_time, true);

  label_date = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_date, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x999999));
  lv_obj_align(label_date, label_time, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);

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

WatchFaceFuzzy::~WatchFaceFuzzy() {
  lv_task_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
}

void WatchFaceFuzzy::Refresh() {
  statusIcons.Update();

  notificationState = notificationManager.AreNewNotificationsAvailable();
  if (notificationState.IsUpdated()) {
    lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(notificationState.Get()));
  }

  currentDateTime = std::chrono::time_point_cast<std::chrono::minutes>(dateTimeController.CurrentDateTime());

  if (currentDateTime.IsUpdated()) {
    uint8_t hour = dateTimeController.Hours();
    uint8_t minute = dateTimeController.Minutes();

    printTimeWords(static_cast<int>(hour), static_cast<int>(minute));

    currentDate = std::chrono::time_point_cast<days>(currentDateTime.Get());
    if (currentDate.IsUpdated()) {
      uint16_t year = dateTimeController.Year();
      uint8_t day = dateTimeController.Day();
      lv_label_set_text_fmt(label_date,
                            "%s %d %s %d",
                            dateTimeController.DayOfWeekShortToString(),
                            day,
                            dateTimeController.MonthShortToString(),
                            year);
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

char const* WatchFaceFuzzy::mods[] = {"", "five", "ten", "quarter", "twenty", "twenty five", "half"};
char const* WatchFaceFuzzy::nums[] = {"twelve", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve"};

void WatchFaceFuzzy::printTimeWords(int h, int m) {
  const char* mod;
  if (m  <= 30) {
    mod = mods[m / 5];
  } else {
    mod = mods[(60-m) / 5];
  }
  h  = (h % 12);

  if (m == 0 || m < 4 || m > 56) {
    sprintf(timeStr, "#ffffff %s#\n#808080 o' clock#", nums[h]);

  }

  else if (m <= 32) {
    sprintf(timeStr, "#ffffff %s#\n#808080 past# #FFFFFF %s#", mod, nums[h]);
  }

  else if (m > 32) {
    sprintf(timeStr, "#ffffff %s#\n#808080 to# #FFFFFF %s#", mod, nums[(h % 12) + 1]);
  }

  lv_label_set_text(label_time, timeStr);
}
