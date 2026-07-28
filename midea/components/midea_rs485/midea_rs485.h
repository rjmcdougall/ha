#pragma once

#include <cmath>

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/climate/climate.h"
#include <esp32_midea_RS485.h>

namespace esphome {
namespace midea_rs485 {

// A native ESPHome climate entity that drives a Midea heat pump over the XYE
// (RS485) bus. Home Assistant renders this as a full thermostat card: HVAC
// mode, target temperature, current temperature, fan speed and swing.
//
// The extra return/coil/outdoor temperatures and the aux-heat / sleep / vent /
// lock toggles are exposed as separate sensor/switch entities wired in via the
// setters below.
class MideaRS485Component : public climate::Climate, public PollingComponent {
 public:
  // --- Configuration setters (called from generated code) ---
  void set_pins(uint8_t di_pin, uint8_t ro_pin, uint8_t de_pin) {
    di_pin_ = di_pin;
    ro_pin_ = ro_pin;
    de_pin_ = de_pin;
  }
  void set_ids(uint8_t master_id, uint8_t slave_id) {
    master_id_ = master_id;
    slave_id_ = slave_id;
  }
  void set_timing(uint8_t send_time, uint8_t timeout) {
    send_time_ = send_time;
    timeout_ = timeout;
  }

  // --- Diagnostic sensor setters ---
  void set_t1_sensor(sensor::Sensor *s) { t1_sensor_ = s; }
  void set_t2a_sensor(sensor::Sensor *s) { t2a_sensor_ = s; }
  void set_t2b_sensor(sensor::Sensor *s) { t2b_sensor_ = s; }
  void set_t3_sensor(sensor::Sensor *s) { t3_sensor_ = s; }
  void set_not_responding_sensor(sensor::Sensor *s) { not_responding_sensor_ = s; }

  // --- Auxiliary switch setters (features with no climate equivalent) ---
  void set_aux_heat_switch(switch_::Switch *s) { aux_heat_switch_ = s; }
  void set_echo_sleep_switch(switch_::Switch *s) { echo_sleep_switch_ = s; }
  void set_vent_switch(switch_::Switch *s) { vent_switch_ = s; }
  void set_lock_switch(switch_::Switch *s) { lock_switch_ = s; }

  float get_setup_priority() const override { return esphome::setup_priority::AFTER_WIFI; }

  void setup() override {
    ESP32_Midea_RS485.begin(&Serial2, ro_pin_, di_pin_, de_pin_,
                             master_id_, slave_id_, send_time_, timeout_);

    // Auxiliary toggles are plain switches; forward user changes to the AC.
    // updating_internal_ guards against the feedback loop created when we
    // publish_state() back to these switches from update().
    if (aux_heat_switch_) {
      aux_heat_switch_->add_on_state_callback([this](bool value) {
        if (!updating_internal_)
          ESP32_Midea_RS485.SetAuxHeat_Turbo(value ? MIDEA_AC_ACTIVE : MIDEA_AC_INACTIVE);
      });
    }
    if (echo_sleep_switch_) {
      echo_sleep_switch_->add_on_state_callback([this](bool value) {
        if (!updating_internal_)
          ESP32_Midea_RS485.SetEcho_Sleep(value ? MIDEA_AC_ACTIVE : MIDEA_AC_INACTIVE);
      });
    }
    if (vent_switch_) {
      vent_switch_->add_on_state_callback([this](bool value) {
        if (!updating_internal_)
          ESP32_Midea_RS485.SetVent(value ? MIDEA_AC_ACTIVE : MIDEA_AC_INACTIVE);
      });
    }
    // Lock/unlock acts immediately, not deferred to the next update cycle.
    if (lock_switch_) {
      lock_switch_->add_on_state_callback([this](bool value) {
        if (value)
          ESP32_Midea_RS485.Lock();
        else
          ESP32_Midea_RS485.Unlock();
      });
    }
  }

  // Advertise what this thermostat can do. Visual min/max/step defaults here are
  // overridden by any `visual:` block in the YAML.
  climate::ClimateTraits traits() override {
    auto traits = climate::ClimateTraits();
    // ESPHome 2026.x replaced the set_supports_* booleans with feature flags.
    traits.set_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
    traits.set_supported_modes({
        climate::CLIMATE_MODE_OFF,
        climate::CLIMATE_MODE_HEAT_COOL,  // Midea "Auto"
        climate::CLIMATE_MODE_COOL,
        climate::CLIMATE_MODE_HEAT,
        climate::CLIMATE_MODE_DRY,
        climate::CLIMATE_MODE_FAN_ONLY,
    });
    traits.set_supported_fan_modes({
        climate::CLIMATE_FAN_AUTO,
        climate::CLIMATE_FAN_LOW,
        climate::CLIMATE_FAN_MEDIUM,
        climate::CLIMATE_FAN_HIGH,
    });
    traits.set_supported_swing_modes({
        climate::CLIMATE_SWING_OFF,
        climate::CLIMATE_SWING_VERTICAL,
    });
    traits.set_visual_min_temperature(16);
    traits.set_visual_max_temperature(32);
    traits.set_visual_temperature_step(1);
    return traits;
  }

  // Handle a command from Home Assistant. The library queues the change and
  // transmits it on the next Update() cycle; we optimistically reflect it so the
  // UI is responsive, and the next poll re-syncs from the AC's actual state.
  void control(const climate::ClimateCall &call) override {
    if (call.get_mode().has_value()) {
      climate::ClimateMode mode = *call.get_mode();
      switch (mode) {
        case climate::CLIMATE_MODE_OFF:       ESP32_Midea_RS485.SetMode(MIDEA_AC_OPMODE_OFF);  break;
        case climate::CLIMATE_MODE_HEAT_COOL: ESP32_Midea_RS485.SetMode(MIDEA_AC_OPMODE_AUTO); break;
        case climate::CLIMATE_MODE_COOL:      ESP32_Midea_RS485.SetMode(MIDEA_AC_OPMODE_COOL); break;
        case climate::CLIMATE_MODE_HEAT:      ESP32_Midea_RS485.SetMode(MIDEA_AC_OPMODE_HEAT); break;
        case climate::CLIMATE_MODE_DRY:       ESP32_Midea_RS485.SetMode(MIDEA_AC_OPMODE_DRY);  break;
        case climate::CLIMATE_MODE_FAN_ONLY:  ESP32_Midea_RS485.SetMode(MIDEA_AC_OPMODE_FAN);  break;
        default: break;
      }
      this->mode = mode;
    }

    if (call.get_target_temperature().has_value()) {
      float t = *call.get_target_temperature();
      ESP32_Midea_RS485.SetTemp((uint8_t) lroundf(t));
      this->target_temperature = t;
    }

    if (call.get_fan_mode().has_value()) {
      climate::ClimateFanMode fan = *call.get_fan_mode();
      switch (fan) {
        case climate::CLIMATE_FAN_AUTO:   ESP32_Midea_RS485.SetFanMode(MIDEA_AC_FANMODE_AUTO);   break;
        case climate::CLIMATE_FAN_LOW:    ESP32_Midea_RS485.SetFanMode(MIDEA_AC_FANMODE_LOW);    break;
        case climate::CLIMATE_FAN_MEDIUM: ESP32_Midea_RS485.SetFanMode(MIDEA_AC_FANMODE_MEDIUM); break;
        case climate::CLIMATE_FAN_HIGH:   ESP32_Midea_RS485.SetFanMode(MIDEA_AC_FANMODE_HIGH);   break;
        default: break;
      }
      this->fan_mode = fan;
    }

    if (call.get_swing_mode().has_value()) {
      climate::ClimateSwingMode swing = *call.get_swing_mode();
      ESP32_Midea_RS485.SetSwing(swing == climate::CLIMATE_SWING_VERTICAL ? MIDEA_AC_ACTIVE
                                                                          : MIDEA_AC_INACTIVE);
      this->swing_mode = swing;
    }

    this->publish_state();
  }

  void update() override {
    ESP32_Midea_RS485.Update();

    if (ESP32_Midea_RS485.State.ACNotResponding == 0) {
      updating_internal_ = true;
      publish_ac_state_();
      updating_internal_ = false;
    }

    if (t1_sensor_) t1_sensor_->publish_state(ESP32_Midea_RS485.State.T1Temp);
    if (t2a_sensor_) t2a_sensor_->publish_state(ESP32_Midea_RS485.State.T2ATemp);
    if (t2b_sensor_) t2b_sensor_->publish_state(ESP32_Midea_RS485.State.T2BTemp);
    if (t3_sensor_) t3_sensor_->publish_state(ESP32_Midea_RS485.State.T3Temp);
    if (not_responding_sensor_)
      not_responding_sensor_->publish_state(ESP32_Midea_RS485.State.ACNotResponding);
  }

 protected:
  // Read the AC's actual reported state and publish it to the climate entity and
  // the auxiliary switches.
  void publish_ac_state_() {
    switch (ESP32_Midea_RS485.State.OpMode) {
      case MIDEA_AC_OPMODE_OFF:  this->mode = climate::CLIMATE_MODE_OFF;       break;
      case MIDEA_AC_OPMODE_AUTO: this->mode = climate::CLIMATE_MODE_HEAT_COOL; break;
      case MIDEA_AC_OPMODE_COOL: this->mode = climate::CLIMATE_MODE_COOL;      break;
      case MIDEA_AC_OPMODE_HEAT: this->mode = climate::CLIMATE_MODE_HEAT;      break;
      case MIDEA_AC_OPMODE_DRY:  this->mode = climate::CLIMATE_MODE_DRY;       break;
      case MIDEA_AC_OPMODE_FAN:  this->mode = climate::CLIMATE_MODE_FAN_ONLY;  break;
      default: break;
    }

    switch (ESP32_Midea_RS485.State.FanMode) {
      case MIDEA_AC_FANMODE_AUTO:   this->fan_mode = climate::CLIMATE_FAN_AUTO;   break;
      case MIDEA_AC_FANMODE_LOW:    this->fan_mode = climate::CLIMATE_FAN_LOW;    break;
      case MIDEA_AC_FANMODE_MEDIUM: this->fan_mode = climate::CLIMATE_FAN_MEDIUM; break;
      case MIDEA_AC_FANMODE_HIGH:   this->fan_mode = climate::CLIMATE_FAN_HIGH;   break;
      default: break;
    }

    bool swing_on = (ESP32_Midea_RS485.State.OperatingFlags & 0x04) > 0;
    this->swing_mode = swing_on ? climate::CLIMATE_SWING_VERTICAL : climate::CLIMATE_SWING_OFF;

    if (ESP32_Midea_RS485.State.SetTemp > 0)
      this->target_temperature = (float) ESP32_Midea_RS485.State.SetTemp;
    this->current_temperature = (float) ESP32_Midea_RS485.State.T1Temp;

    this->publish_state();

    if (aux_heat_switch_)
      aux_heat_switch_->publish_state((ESP32_Midea_RS485.State.OperatingFlags & 0x02) > 0);
    if (echo_sleep_switch_)
      echo_sleep_switch_->publish_state((ESP32_Midea_RS485.State.OperatingFlags & 0x01) > 0);
    if (vent_switch_)
      vent_switch_->publish_state((ESP32_Midea_RS485.State.OperatingFlags & 0x88) > 0);
  }

  uint8_t di_pin_{16}, ro_pin_{17}, de_pin_{4};
  uint8_t master_id_{0}, slave_id_{0};
  uint8_t send_time_{40}, timeout_{100};

  sensor::Sensor *t1_sensor_{nullptr};
  sensor::Sensor *t2a_sensor_{nullptr};
  sensor::Sensor *t2b_sensor_{nullptr};
  sensor::Sensor *t3_sensor_{nullptr};
  sensor::Sensor *not_responding_sensor_{nullptr};

  switch_::Switch *aux_heat_switch_{nullptr};
  switch_::Switch *echo_sleep_switch_{nullptr};
  switch_::Switch *vent_switch_{nullptr};
  switch_::Switch *lock_switch_{nullptr};

  bool updating_internal_{false};
};

}  // namespace midea_rs485
}  // namespace esphome
