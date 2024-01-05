#include "displayapp/screens/WatchFaceFuzzy.h"

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

WatchFaceFuzzy::WatchFaceFuzzy(Controllers::DateTime& dateTimeController,
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

  statusIcons.Create();

  notificationIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(notificationIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_LIME);
  lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(false));
  lv_obj_align(notificationIcon, nullptr, LV_ALIGN_IN_TOP_LEFT, 0, 0);

  label_date = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(label_date, lv_scr_act(), LV_ALIGN_CENTER, 0, 60);
  lv_obj_set_style_local_text_color(label_date, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x999999));

  label_time = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_extrabold_compressed);

  lv_obj_align(label_time, lv_scr_act(), LV_ALIGN_IN_RIGHT_MID, 0, 0);

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

    /* Begin difference from WatchFaceDigital*/
    std::string hourStr, timeStr;
    auto sector = minute / 5 + (minute % 5 > 2);
    if (sector == 12) {
      hour = (hour + 1) % 12;
      sector = 0;
    }

    timeStr = timeSectors[sector];
    if (timeStr.find("%1") != std::string::npos) {
      hour = (hour + 1) % 12;
    }
    hourStr = std::string("#") + timeAccent + " " + hourNames[hour] + "#";
    timeStr.replace(timeStr.find("%"), 2, hourStr);

    lv_label_set_text(label_time, timeStr.c_str());
    lv_obj_align(label_time, lv_scr_act(), LV_ALIGN_CENTER, 0, 0);
    /* End difference from WatchFaceDigital*/

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

/* Inspired by XFCE4-panel's fuzzy clock.
 *
 *      https://salsa.debian.org/xfce-team/desktop/xfce4-panel/-/blob/debian/master/plugins/clock/clock-fuzzy.c
 *
 * Strings contain either a `%0` or a `%1`, indicating the position of
 * the `hour` or `hour+1`, respectively.
 */
const char* WatchFaceFuzzy::timeSectors[] = {
  "%0\no'clock",
  "five past\n%0",
  "ten past\n%0",
  "quarter\npast\n%0",
  "twenty\npast\n%0",
  "twenty\nfive past\n%0",
  "half past\n%0",
  "twenty\nfive to\n%1",
  "twenty\nto %1",
  "quarter\nto %1",
  "ten to\n%1",
  "five to\n%1",
};
const char* WatchFaceFuzzy::hourNames[] = {
  "twelve",
  "one",
  "two",
  "three",
  "four",
  "five",
  "six",
  "seven",
  "eight",
  "nine",
  "ten",
  "eleven",
};

/* Once i18n is implemented, new languages can be introduced like this:
 *
 * const char* ca-ES_sectors[] = {
 *   "%0\nen punt",
 *   "%0\ni cinc",
 *   "%0\ni deu",
 *   "%0\ni quart",
 *   "%0\ni vint",
 *   "%0\ni vint-\ni-cinc",
 *   "%0\ni mitja",
 *   "%1\nmenys\nvint-\ni-cinc",
 *   "%1\nmenys\nvint",
 *   "%1\nmenys\nquart",
 *   "%1\nmenys deu",
 *   "%1\nmenys\ncinc",
 * };
 * const char* ca-ES_hourNames[] = {
 *   "les dotze",
 *   "la una",
 *   "les dues",
 *   "les tres",
 *   "les\nquatre",
 *   "les cinc",
 *   "les sis",
 *   "les set",
 *   "les vuit",
 *   "les nou",
 *   "les deu",
 *   "les onze",
 * };
 *
 * const char* es-ES_sectors[] = {
 *   "%0\nen punto",
 *   "%0\ny cinco",
 *   "%0\ny diez",
 *   "%0\ny cuarto",
 *   "%0\ny veinte",
 *   "%0\ny veinti\ncinco",
 *   "%0\ny media",
 *   "%1\nmenos\nveinti\ncinco",
 *   "%1\nmenos\nveinte",
 *   "%1\nmenos\ncuarto",
 *   "%1\nmenos\ndiez",
 *   "%1\nmenos\ncinco",
 * };
 * const char* es-ES_hourNames[] = {
 *   "las doce",
 *   "la una",
 *   "las dos",
 *   "las tres",
 *   "las\ncuatro",
 *   "las cinco",
 *   "las seis",
 *   "las siete",
 *   "las ocho",
 *   "las nueve",
 *   "las diez",
 *   "las once",
 * };
 *
 * char* it-IT_sectors[] = {
 *   "%0\nin punto",
 *   "%0 e cinque",
 *   "%0 e dieci",
 *   "%0 e un quarto",
 *   "%0 e venti",
 *   "%0 e venti cinque",
 *   "%0 e mezza",
 *   "%0 e trenta cinque",
 *   "%1 meno venti",
 *   "%1 meno un quarto",
 *   "%1 meno dieci",
 *   "%1 meno cinque",
 * };
 * const char* it-IT_hourNames[] = {
 *   "dodici",
 *   "una",
 *   "due",
 *   "tre",
 *   "quattro",
 *   "cinque",
 *   "sei",
 *   "sette",
 *   "otto",
 *   "nove",
 *   "dieci",
 *   "undici",
 * };
 *
 * const char* de_DE_sectors[] = {
 *   "%0 Uhr",
 *   "Fünf nach %0",
 *   "Zehn nach %0",
 *   "Viertel nach %0",
 *   "Zwanzig nach %0",
 *   "Fünf vor halb %1",
 *   "Halb %1",
 *   "Fünf nach halb %1",
 *   "Zwanzig vor %1",
 *   "Viertel vor %1",
 *   "Zehn vor %1",
 *   "Fünf vor %1",
 * };
 * const char* de_DE_hourNames[] = {
 *   "Zwölf",
 *   "Eins", // TODO: "Ein" in "Ein Uhr"
 *   "Zwei",
 *   "Drei",
 *   "Vier",
 *   "Fünf",
 *   "Sechs",
 *   "Sieben",
 *   "Acht",
 *   "Neun",
 *   "Zehn",
 *   "Elf",
 * };
 */