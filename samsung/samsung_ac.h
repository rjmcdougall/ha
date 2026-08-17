/*
A custom `climate` module for ESPHome for controlling Samsung reverse-cycle
air conditioners.

Basic usage in the ESPHome YAML file:

esphome:
 name: <name for your config>
 platform: ESP8266
 board: <your board>
  includes:
    - path_to/samsung_climate.h
  libraries:

climate:
- platform: custom
  lambda: |-
    auto samsung_climate = new SamsungClimate();
    App.register_component(samsung_climate);
    return {samsung_climate};
  climates:
    - name: "<name for your climate control>"
*/

#include "esphome.h"
#include "ESP32_Samsung.h"

class SamsungClimate : public Component, public Climate {
 public:
//  ESP32_Samsung ac;
  void setup() override {
    // This will be called by App.setup()
    // Tested working
    ESP_LOGCONFIG("samsung", "Setting up samsung...");
    //ESP32_Samsung.begin(19, 17, 16);
    ESP32_Samsung.begin(17, 16, 19);

    //ESP32_Samsung.begin();
    // Some sensible defaults.
    ESP32_Samsung.setFan(kSamsungFanAuto);

   // restore set points
   auto restore = this->restore_state_();
   if (restore.has_value()) {
     restore->apply(this);
   } else {
     // restore from defaults
     this->mode = climate::CLIMATE_MODE_OFF;
     // initialize target temperature to some value so that it's not NAN
     this->target_temperature =
         roundf(clamp(this->current_temperature, (float)kSamsungMinTemp, (float)kSamsungMaxTemp));
     this->fan_mode = climate::CLIMATE_FAN_AUTO;
   }
   // Never send nan to HA
   if (std::isnan(this->target_temperature))
     this->target_temperature = 24;
  }

  ClimateTraits traits() override  {
    ESP_LOGD("samsung", "traits()...");
    auto traits = ClimateTraits();
    traits.set_supports_current_temperature(false);
//    traits.set_supports_current_temperature(this->sensor_ != nullptr);
    traits.set_supported_modes({climate::CLIMATE_MODE_OFF, climate::CLIMATE_MODE_HEAT_COOL});
    traits.add_supported_mode(climate::CLIMATE_MODE_COOL);
    traits.add_supported_mode(climate::CLIMATE_MODE_HEAT);
    traits.add_supported_mode(climate::CLIMATE_MODE_DRY);
    traits.add_supported_mode(climate::CLIMATE_MODE_FAN_ONLY);
//    traits.set_supports_auto_mode(true);
//    traits.set_supports_cool_mode(true);
//    traits.set_supports_heat_mode(true);
    traits.set_supports_two_point_target_temperature(false);
    traits.set_visual_min_temperature(kSamsungMinTemp);
    traits.set_visual_max_temperature(kSamsungMaxTemp);
    traits.set_visual_temperature_step(1);
    traits.set_supported_fan_modes({CLIMATE_FAN_ON, CLIMATE_FAN_OFF, CLIMATE_FAN_AUTO, CLIMATE_FAN_LOW, CLIMATE_FAN_MEDIUM, CLIMATE_FAN_HIGH});

    return traits;
  }

  void control(const ClimateCall &call) override {
    ESP_LOGD("samsung", "control()...");

    if (call.get_mode().has_value()) {
      // User requested mode change
      this->mode = *call.get_mode();
    }
    if (call.get_target_temperature().has_value()) {
      // User requested target temperature change
      this->target_temperature = *call.get_target_temperature();
    }
    this->transmit_state_();
    this->publish_state();
  }
 protected:
  // Set the GPIO pins for RS485
  void transmit_state_() {
    ESP32_Samsung.on();
    // Handle settings.
    switch (this->mode) {
    case CLIMATE_MODE_AUTO:
      ESP32_Samsung.setMode(kSamsungModeAuto);
      break;
    case CLIMATE_MODE_HEAT:
      ESP32_Samsung.setMode(kSamsungModeHeat);
      break;
    case CLIMATE_MODE_COOL:
      ESP32_Samsung.setMode(kSamsungModeCool);
      break;
    case CLIMATE_MODE_OFF:
    default:
      ESP32_Samsung.off();
      break;
    }
    auto t = (uint8_t) roundf(this->target_temperature);
    ESP32_Samsung.setTemp(t);
    ESP32_Samsung.send();
  }
};
