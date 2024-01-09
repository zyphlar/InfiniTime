#pragma once

#include <cstdint>
#include <lvgl/lvgl.h>
#include "components/datetime/DateTimeController.h"
#include "components/settings/Settings.h"
#include "displayapp/widgets/Counter.h"
#include "displayapp/screens/Screen.h"
#include "displayapp/widgets/DotIndicator.h"
#include "displayapp/screens/settings/SettingSetDateTime.h"

namespace Pinetime {
  namespace Applications {
    namespace Screens {
      class SettingLocation : public Screen {
      public:
        SettingLocation(Pinetime::Controllers::Settings& settingsController);
        ~SettingLocation() override;

        void UpdateScreen();

      private:
        Controllers::Settings& settingsController;

        Widgets::Counter latCounter = Widgets::Counter(-90, 90, jetbrains_mono_42);
        Widgets::Counter longCounter = Widgets::Counter(-180, 180, jetbrains_mono_42);
        Widgets::Counter tzCounter = Widgets::Counter(-12, 12, jetbrains_mono_42);
      };
    }
  }
}
