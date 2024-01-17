#include "displayapp/screens/WatchFaceAnalog.h"
#include <cmath>
#include <lvgl/lvgl.h>
#include <nrf_log.h>
#include "displayapp/screens/BatteryIcon.h"
#include "displayapp/screens/BleIcon.h"
#include "displayapp/screens/Symbols.h"
#include "displayapp/screens/NotificationIcon.h"
#include "components/settings/Settings.h"
#include "displayapp/InfiniTimeTheme.h"
#include "sunset/src/sunset.h"

using namespace Pinetime::Applications::Screens;

namespace {
  int16_t HourLength = 70;
  constexpr int16_t MinuteLength = 90;
  constexpr int16_t SecondLength = 110;
  constexpr int16_t SunDialVerticalOffset = 40;

  // sin(90) = 1 so the value of _lv_trigo_sin(90) is the scaling factor
  const auto LV_TRIG_SCALE = _lv_trigo_sin(90);
  const lv_color_t DARK_GRAY = lv_color_make(48, 48, 48);
  const lv_color_t DARK_ORANGE = lv_color_make(48, 26, 0);

  int16_t Cosine(int16_t angle) {
    return _lv_trigo_sin(angle + 90);
  }

  int16_t Sine(int16_t angle) {
    return _lv_trigo_sin(angle);
  }

  int16_t CoordinateXRelocate(int16_t x) {
    return (x + LV_HOR_RES / 2);
  }

  int16_t CoordinateYRelocate(int16_t y) {
    return std::abs(y - LV_HOR_RES / 2);
  }

  lv_point_t CoordinateRelocate(int16_t radius, int16_t angle) {
    return lv_point_t {.x = CoordinateXRelocate(radius * static_cast<int32_t>(Sine(angle)) / LV_TRIG_SCALE),
                       .y = CoordinateYRelocate(radius * static_cast<int32_t>(Cosine(angle)) / LV_TRIG_SCALE)};
  }

  void printKoku(const char* str, uint8_t i, bool big){
    lv_obj_t* koku = lv_label_create(lv_scr_act(), NULL);
    lv_label_set_align(koku, LV_LABEL_ALIGN_CENTER);
    lv_label_set_text(koku, str);
    lv_point_t pt;
    if (big) {
      pt = CoordinateRelocate(LV_HOR_RES/2-20, i*30);
      lv_obj_set_style_local_text_font(koku, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &noto_serif_cjk_20);
    } else {
      pt = CoordinateRelocate(LV_HOR_RES/2-45, i*30);
      lv_obj_set_style_local_text_font(koku, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &noto_serif_cjk_15);
    }
    lv_obj_align(koku, NULL, LV_ALIGN_CENTER, LV_HOR_RES/2-pt.x-10, LV_HOR_RES/2-pt.y);
    lv_obj_set_style_local_text_color(koku, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  }

}

WatchFaceAnalog::WatchFaceAnalog(Controllers::DateTime& dateTimeController,
                                 const Controllers::Battery& batteryController,
                                 const Controllers::Ble& bleController,
                                 Controllers::NotificationManager& notificationManager,
                                 Controllers::Settings& settingsController)
  : currentDateTime {{}},
    batteryIcon(true),
    dateTimeController {dateTimeController},
    batteryController {batteryController},
    bleController {bleController},
    notificationManager {notificationManager},
    settingsController {settingsController} {

  sHour = 99;
  sMinute = 99;
  sSecond = 99;

  location = settingsController.GetLocation();

  // begin sundial watch face
  if (settingsController.GetClockType() == Controllers::Settings::ClockType::Fuzzy) {
    major_scales = lv_linemeter_create(lv_scr_act(), nullptr);
    lv_linemeter_set_scale(major_scales, 360, 13);
    lv_linemeter_set_angle_offset(major_scales, 15);
    lv_obj_set_size(major_scales, 240, 240);
    lv_obj_align(major_scales, nullptr, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_local_bg_opa(major_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
    lv_obj_set_style_local_scale_width(major_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, 60);
    lv_obj_set_style_local_scale_end_line_width(major_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, 1);
    lv_obj_set_style_local_scale_end_color(major_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);

    axis = lv_arc_create(lv_scr_act(), NULL);
    lv_obj_set_size(axis, 40, 40);
    lv_arc_set_bg_angles(axis, 0, 360);
    lv_arc_set_end_angle(axis, 360);
    lv_obj_align(axis, NULL, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_local_line_color(axis, LV_ARC_PART_INDIC, LV_STATE_DEFAULT, LV_COLOR_WHITE);

    const char* kokuZodiac[] = {"子","丑","寅","卯","辰","巳","午","未","申","酉","戌","亥"};
    const char* kokuNums[] = {"九","八","七","六","五","四"};

    for (int i=0;i<12;i++) {
      printKoku(kokuZodiac[i], i, true);
      printKoku(kokuNums[i%6], i, false);
    }
  } else {
    minor_scales = lv_linemeter_create(lv_scr_act(), nullptr);
    lv_linemeter_set_scale(minor_scales, 300, 51);
    lv_linemeter_set_angle_offset(minor_scales, 180);
    lv_obj_set_size(minor_scales, 240, 240);
    lv_obj_align(minor_scales, nullptr, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_local_bg_opa(minor_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
    lv_obj_set_style_local_scale_width(minor_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, 4);
    lv_obj_set_style_local_scale_end_line_width(minor_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, 1);
    lv_obj_set_style_local_scale_end_color(minor_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GRAY);

    major_scales = lv_linemeter_create(lv_scr_act(), nullptr);
    lv_linemeter_set_scale(major_scales, 300, 11);
    lv_linemeter_set_angle_offset(major_scales, 180);
    lv_obj_set_size(major_scales, 240, 240);
    lv_obj_align(major_scales, nullptr, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_local_bg_opa(major_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
    lv_obj_set_style_local_scale_width(major_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, 6);
    lv_obj_set_style_local_scale_end_line_width(major_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, 4);
    lv_obj_set_style_local_scale_end_color(major_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);

    large_scales = lv_linemeter_create(lv_scr_act(), nullptr);
    lv_linemeter_set_scale(large_scales, 180, 3);
    lv_linemeter_set_angle_offset(large_scales, 180);
    lv_obj_set_size(large_scales, 240, 240);
    lv_obj_align(large_scales, nullptr, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_local_bg_opa(large_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
    lv_obj_set_style_local_scale_width(large_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, 20);
    lv_obj_set_style_local_scale_end_line_width(large_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, 4);
    lv_obj_set_style_local_scale_end_color(large_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_AQUA);

    twelve = lv_label_create(lv_scr_act(), nullptr);
    lv_label_set_align(twelve, LV_LABEL_ALIGN_CENTER);
    lv_label_set_text_static(twelve, "12");
    lv_obj_set_pos(twelve, 110, 10);
    lv_obj_set_style_local_text_color(twelve, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_AQUA);
  }

  batteryIcon.Create(lv_scr_act());
  lv_obj_align(batteryIcon.GetObject(), nullptr, LV_ALIGN_IN_TOP_RIGHT, 0, 0);

  plugIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(plugIcon, Symbols::plug);
  lv_obj_align(plugIcon, nullptr, LV_ALIGN_IN_TOP_RIGHT, 0, 0);

  bleIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(bleIcon, "");
  lv_obj_align(bleIcon, nullptr, LV_ALIGN_IN_TOP_RIGHT, -30, 0);

  notificationIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(notificationIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_LIME);
  lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(false));
  lv_obj_align(notificationIcon, nullptr, LV_ALIGN_IN_TOP_LEFT, 0, 0);

  // Date - Day / Week day
  label_date_day = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_date_day, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::orange);
  lv_label_set_text_fmt(label_date_day, "%s\n%02i", dateTimeController.DayOfWeekShortToString(), dateTimeController.Day());
  lv_label_set_align(label_date_day, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(label_date_day, nullptr, LV_ALIGN_CENTER, 50, 0);

  minute_body = lv_line_create(lv_scr_act(), nullptr);
  minute_body_trace = lv_line_create(lv_scr_act(), nullptr);
  hour_body = lv_line_create(lv_scr_act(), nullptr);
  hour_body_trace = lv_line_create(lv_scr_act(), nullptr);
  second_body = lv_line_create(lv_scr_act(), nullptr);

  lv_style_init(&second_line_style);
  lv_style_set_line_width(&second_line_style, LV_STATE_DEFAULT, 3);
  lv_style_set_line_color(&second_line_style, LV_STATE_DEFAULT, LV_COLOR_RED);
  lv_style_set_line_rounded(&second_line_style, LV_STATE_DEFAULT, true);
  lv_obj_add_style(second_body, LV_LINE_PART_MAIN, &second_line_style);

  lv_style_init(&minute_line_style);
  lv_style_set_line_width(&minute_line_style, LV_STATE_DEFAULT, 7);
  lv_style_set_line_color(&minute_line_style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_style_set_line_rounded(&minute_line_style, LV_STATE_DEFAULT, true);
  lv_obj_add_style(minute_body, LV_LINE_PART_MAIN, &minute_line_style);

  lv_style_init(&minute_line_style_trace);
  lv_style_set_line_width(&minute_line_style_trace, LV_STATE_DEFAULT, 3);
  lv_style_set_line_color(&minute_line_style_trace, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_style_set_line_rounded(&minute_line_style_trace, LV_STATE_DEFAULT, false);
  lv_obj_add_style(minute_body_trace, LV_LINE_PART_MAIN, &minute_line_style_trace);

  lv_style_init(&hour_line_style);
  lv_style_set_line_width(&hour_line_style, LV_STATE_DEFAULT, 7);
  lv_style_set_line_color(&hour_line_style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_style_set_line_rounded(&hour_line_style, LV_STATE_DEFAULT, true);
  lv_obj_add_style(hour_body, LV_LINE_PART_MAIN, &hour_line_style);

  lv_style_init(&hour_line_style_trace);
  lv_style_set_line_width(&hour_line_style_trace, LV_STATE_DEFAULT, 3);
  lv_style_set_line_color(&hour_line_style_trace, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_style_set_line_rounded(&hour_line_style_trace, LV_STATE_DEFAULT, false);
  lv_obj_add_style(hour_body_trace, LV_LINE_PART_MAIN, &hour_line_style_trace);

  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);

  Refresh();
}

WatchFaceAnalog::~WatchFaceAnalog() {
  lv_task_del(taskRefresh);

  lv_style_reset(&hour_line_style);
  lv_style_reset(&hour_line_style_trace);
  lv_style_reset(&minute_line_style);
  lv_style_reset(&minute_line_style_trace);
  lv_style_reset(&second_line_style);

  lv_obj_clean(lv_scr_act());
}


void WatchFaceAnalog::drawWatchFaceWadokei(){
  uint8_t hour = dateTimeController.Hours();
  uint8_t minute = dateTimeController.Minutes();

  if (sHour != hour || sMinute != minute) {
    // sun.setPosition(settings.lat.toFloat(), settings.lon.toFloat(), settings.gmtOffset / 3600);
    sun.setPosition((float)location.latitude, (float)location.longitude, location.tzOffset);

    //from minutes past midnight
    sun.setCurrentDate(dateTimeController.Year(), static_cast<uint8_t>(dateTimeController.Month())+1, dateTimeController.Day());
    sun.setTZOffset(location.tzOffset);

    minutesSunrise = sun.calcSunrise(); //360;
    minutesSunset = sun.calcSunset(); //1080;
    minutesDaytime = (minutesSunset - minutesSunrise);
    minutesNighttime = (1440 - minutesDaytime);

    minutesBeforeSunset = minutesSunset - (hour * 60 + minute); // i.e.zero degrees
    HourLength = 40; // wadokei hand length

    int16_t oldHourAngle;
    auto const angle = (hour * 15 + minute / 2)+180; //24-hr rotation with 0/24 at the bottom

    if(minutesBeforeSunset > 0 && minutesBeforeSunset < minutesDaytime) { // day (after sunrise)
      oldHourAngle = 180.0 * minutesBeforeSunset / minutesDaytime + 90;
    } else { // night (before sunrise or after sunset)
      // lv_style_set_line_color(&hour_line_style, LV_STATE_DEFAULT, DARK_GRAY);
      // lv_style_set_line_color(&hour_line_style_trace, LV_STATE_DEFAULT, DARK_GRAY);
      // lv_obj_set_style_local_scale_end_color(major_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, DARK_GRAY);
      // lv_obj_set_style_local_text_color(label_date_day, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, DARK_ORANGE);
      // lv_obj_set_style_local_text_color(one, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, DARK_GRAY);
      // lv_obj_set_style_local_text_color(twelve, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, DARK_GRAY);

      if(minutesBeforeSunset > minutesDaytime) { // before sunrise
        oldHourAngle = 180.0 * (minutesBeforeSunset - minutesDaytime) / minutesNighttime + 90;
      } else { // after sunset
        oldHourAngle = 180 + 180.0 * minutesBeforeSunset / minutesNighttime + 90;
      }
    }
    NRF_LOG_INFO("oa: %d, a: %d, la: %f, lo: %f, be: %d",
      oldHourAngle,
      angle,
      (float)location.latitude,
      (float)location.longitude,
      minutesBeforeSunset);

    sHour = hour;
    sMinute = minute;

    // hour_point_trace[0] = CoordinateRelocate(HourLength*.75, angle);
    // hour_point_trace[1] = CoordinateRelocate(HourLength, angle);

    hour_point[0] = CoordinateRelocate(0, angle);
    hour_point[1] = CoordinateRelocate(HourLength*.75, angle);

    lv_line_set_points(hour_body, hour_point, 2);
    // lv_line_set_points(hour_body_trace, hour_point_trace, 2);

    lv_label_set_align(twelve, LV_LABEL_ALIGN_CENTER);
    lv_label_set_text_fmt(twelve, "%2d:%02d", hour, minute);
    lv_obj_set_pos(twelve, 0, 0);
    lv_obj_set_style_local_text_color(twelve, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  }
}

void WatchFaceAnalog::UpdateClock() {
  if (settingsController.GetClockType() == Controllers::Settings::ClockType::Fuzzy) {
    drawWatchFaceWadokei();
    return;
  }

  uint8_t hour = dateTimeController.Hours();
  uint8_t minute = dateTimeController.Minutes();
  uint8_t second = dateTimeController.Seconds();

  if (minute>99 && sMinute != minute) { // disable
    auto const angle = minute * 6;
    minute_point[0] = CoordinateRelocate(30, angle);
    minute_point[1] = CoordinateRelocate(MinuteLength, angle);

    minute_point_trace[0] = CoordinateRelocate(5, angle);
    minute_point_trace[1] = CoordinateRelocate(31, angle);

    lv_line_set_points(minute_body, minute_point, 2);
    lv_line_set_points(minute_body_trace, minute_point_trace, 2);
  }

  if (sHour != hour || sMinute != minute) {
    sHour = hour;
    sMinute = minute;
    auto const angle = (hour * 30 + minute / 2);

    hour_point[0] = CoordinateRelocate(30, angle);
    hour_point[1] = CoordinateRelocate(HourLength, angle);

    hour_point_trace[0] = CoordinateRelocate(5, angle);
    hour_point_trace[1] = CoordinateRelocate(31, angle);

    lv_line_set_points(hour_body, hour_point, 2);
    lv_line_set_points(hour_body_trace, hour_point_trace, 2);
  }


  if (second>99 && sSecond != second) { // disable
    sSecond = second;
    auto const angle = second * 6;

    second_point[0] = CoordinateRelocate(-20, angle);
    second_point[1] = CoordinateRelocate(SecondLength, angle);
    lv_line_set_points(second_body, second_point, 2);
  }
}

void WatchFaceAnalog::SetBatteryIcon() {
  auto batteryPercent = batteryPercentRemaining.Get();
  batteryIcon.SetBatteryPercentage(batteryPercent);
}

void WatchFaceAnalog::Refresh() {
  isCharging = batteryController.IsCharging();
  if (isCharging.IsUpdated()) {
    if (isCharging.Get()) {
      lv_obj_set_hidden(batteryIcon.GetObject(), true);
      lv_obj_set_hidden(plugIcon, false);
    } else {
      lv_obj_set_hidden(batteryIcon.GetObject(), false);
      lv_obj_set_hidden(plugIcon, true);
      SetBatteryIcon();
    }
  }
  if (!isCharging.Get()) {
    batteryPercentRemaining = batteryController.PercentRemaining();
    if (batteryPercentRemaining.IsUpdated()) {
      SetBatteryIcon();
    }
  }

  bleState = bleController.IsConnected();
  if (bleState.IsUpdated()) {
    if (bleState.Get()) {
      lv_label_set_text_static(bleIcon, Symbols::bluetooth);
    } else {
      lv_label_set_text_static(bleIcon, "");
    }
  }

  notificationState = notificationManager.AreNewNotificationsAvailable();

  if (notificationState.IsUpdated()) {
    lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(notificationState.Get()));
  }

  currentDateTime = dateTimeController.CurrentDateTime();
  if (currentDateTime.IsUpdated()) {
    UpdateClock();

    currentDate = std::chrono::time_point_cast<days>(currentDateTime.Get());
    if (currentDate.IsUpdated()) {
      if (settingsController.GetClockType() == Controllers::Settings::ClockType::Fuzzy) {
        char const* MonthsString[] = {"--", "IANUARIUS","FEBRUARIUS","MARTIUS","APRILIS","MARTIUSIUNIUS","QUINTILIS","SEXTILIS","SEPTEMBER","OCTOBER","NOVEMBER","DECEMBER"};
        char const* DaysString[] = {"--", "LUNAE", "MARTIS", "MERCURII", "IOVIS", "VENERIS", "SATURNI", "SOLIS"};
        char const* RomanNumeralsString[] = {"--", "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X", "XI", "XII"};
        lv_label_set_text_fmt(label_date_day, "%s\n%s %s",
          DaysString[static_cast<uint8_t>(dateTimeController.DayOfWeek())],
          RomanNumeralsString[static_cast<uint8_t>(dateTimeController.Day())],
          MonthsString[static_cast<uint8_t>(dateTimeController.Month())]);
        lv_obj_align(label_date_day, nullptr, LV_ALIGN_IN_BOTTOM_MID, 0, -2000);
      } else {
        lv_label_set_text_fmt(label_date_day, "%s\n%02i", dateTimeController.DayOfWeekShortToString(), dateTimeController.Day());
      }
    }
  }
}