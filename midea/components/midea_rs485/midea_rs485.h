#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/select/select.h"
#include "esphome/components/number/number.h"
#include "esphome/components/switch/switch.h"
#include <esp32_midea_RS485.h>

namespace esphome {
namespace midea_rs485 {

class MideaRS485Component : public PollingComponent {
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

  // --- Sensor setters ---
  void set_t1_sensor(sensor::Sensor *s) { t1_sensor_ = s; }
  void set_t2a_sensor(sensor::Sensor *s) { t2a_sensor_ = s; }
  void set_t2b_sensor(sensor::Sensor *s) { t2b_sensor_ = s; }
  void set_t3_sensor(sensor::Sensor *s) { t3_sensor_ = s; }
  void set_not_responding_sensor(sensor::Sensor *s) { not_responding_sensor_ = s; }

  // --- Control entity setters ---
  void set_mode_select(select::Select *s) { mode_select_ = s; }
  void set_fan_mode_select(select::Select *s) { fan_mode_select_ = s; }
  void set_temp_number(number::Number *n) { temp_number_ = n; }
  void set_aux_heat_switch(switch_::Switch *s) { aux_heat_switch_ = s; }
  void set_echo_sleep_switch(switch_::Switch *s) { echo_sleep_switch_ = s; }
  void set_swing_switch(switch_::Switch *s) { swing_switch_ = s; }
  void set_vent_switch(switch_::Switch *s) { vent_switch_ = s; }
  void set_lock_switch(switch_::Switch *s) { lock_switch_ = s; }

  float get_setup_priority() const override { return esphome::setup_priority::AFTER_WIFI; }

  void setup() override {
    ESP32_Midea_RS485.begin(&Serial2, ro_pin_, di_pin_, de_pin_,
                             master_id_, slave_id_, send_time_, timeout_);

    // Subscribe to user changes on control entities so we can forward them to the AC.
    // The updating_internal_ guard prevents feedback loops when we publish_state()
    // back to these entities from update().

    if (mode_select_) {
      mode_select_->add_on_state_callback([this](size_t) {
        const std::string &value = mode_select_->state;
        if (!updating_internal_ && value != desired_mode_) {
          desired_mode_ = value;
          update_command_ = true;
        }
      });
    }
    if (fan_mode_select_) {
      fan_mode_select_->add_on_state_callback([this](size_t) {
        const std::string &value = fan_mode_select_->state;
        if (!updating_internal_ && value != desired_fan_mode_) {
          desired_fan_mode_ = value;
          update_command_ = true;
        }
      });
    }
    if (temp_number_) {
      temp_number_->add_on_state_callback([this](float value) {
        if (!updating_internal_ && (uint8_t)value != desired_temp_) {
          desired_temp_ = (uint8_t)value;
          update_command_ = true;
        }
      });
    }
    if (aux_heat_switch_) {
      aux_heat_switch_->add_on_state_callback([this](bool value) {
        if (!updating_internal_ && value != aux_heat_) {
          aux_heat_ = value;
          update_command_ = true;
        }
      });
    }
    if (echo_sleep_switch_) {
      echo_sleep_switch_->add_on_state_callback([this](bool value) {
        if (!updating_internal_ && value != echo_sleep_) {
          echo_sleep_ = value;
          update_command_ = true;
        }
      });
    }
    if (swing_switch_) {
      swing_switch_->add_on_state_callback([this](bool value) {
        if (!updating_internal_ && value != swing_) {
          swing_ = value;
          update_command_ = true;
        }
      });
    }
    if (vent_switch_) {
      vent_switch_->add_on_state_callback([this](bool value) {
        if (!updating_internal_ && value != vent_) {
          vent_ = value;
          update_command_ = true;
        }
      });
    }
    // Lock/unlock acts immediately, not deferred to next update cycle
    if (lock_switch_) {
      lock_switch_->add_on_state_callback([this](bool value) {
        if (value) {
          ESP32_Midea_RS485.Lock();
        } else {
          ESP32_Midea_RS485.Unlock();
        }
      });
    }
  }

  void update() override {
    if (update_command_) {
      apply_desired_state_();
      update_command_ = false;
    }

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
  uint8_t di_pin_{16}, ro_pin_{17}, de_pin_{4};
  uint8_t master_id_{0}, slave_id_{0};
  uint8_t send_time_{40}, timeout_{100};

  sensor::Sensor *t1_sensor_{nullptr};
  sensor::Sensor *t2a_sensor_{nullptr};
  sensor::Sensor *t2b_sensor_{nullptr};
  sensor::Sensor *t3_sensor_{nullptr};
  sensor::Sensor *not_responding_sensor_{nullptr};

  select::Select *mode_select_{nullptr};
  select::Select *fan_mode_select_{nullptr};
  number::Number *temp_number_{nullptr};
  switch_::Switch *aux_heat_switch_{nullptr};
  switch_::Switch *echo_sleep_switch_{nullptr};
  switch_::Switch *swing_switch_{nullptr};
  switch_::Switch *vent_switch_{nullptr};
  switch_::Switch *lock_switch_{nullptr};

  bool update_command_{false};
  bool updating_internal_{false};
  std::string desired_mode_{"Unknown"};
  std::string desired_fan_mode_{"Unknown"};
  uint8_t desired_temp_{16};
  bool aux_heat_{false};
  bool echo_sleep_{false};
  bool swing_{false};
  bool vent_{false};

  void apply_desired_state_() {
    if (desired_mode_ == "Auto")       ESP32_Midea_RS485.SetMode(MIDEA_AC_OPMODE_AUTO);
    else if (desired_mode_ == "Off")   ESP32_Midea_RS485.SetMode(MIDEA_AC_OPMODE_OFF);
    else if (desired_mode_ == "Cool")  ESP32_Midea_RS485.SetMode(MIDEA_AC_OPMODE_COOL);
    else if (desired_mode_ == "Heat")  ESP32_Midea_RS485.SetMode(MIDEA_AC_OPMODE_HEAT);
    else if (desired_mode_ == "Dry")   ESP32_Midea_RS485.SetMode(MIDEA_AC_OPMODE_DRY);
    else if (desired_mode_ == "Fan")   ESP32_Midea_RS485.SetMode(MIDEA_AC_OPMODE_FAN);

    if (desired_fan_mode_ == "Auto")       ESP32_Midea_RS485.SetFanMode(MIDEA_AC_FANMODE_AUTO);
    else if (desired_fan_mode_ == "High")   ESP32_Midea_RS485.SetFanMode(MIDEA_AC_FANMODE_HIGH);
    else if (desired_fan_mode_ == "Medium") ESP32_Midea_RS485.SetFanMode(MIDEA_AC_FANMODE_MEDIUM);
    else if (desired_fan_mode_ == "Low")    ESP32_Midea_RS485.SetFanMode(MIDEA_AC_FANMODE_LOW);

    ESP32_Midea_RS485.SetTemp(desired_temp_);
    ESP32_Midea_RS485.SetAuxHeat_Turbo(aux_heat_ ? MIDEA_AC_ACTIVE : MIDEA_AC_INACTIVE);
    ESP32_Midea_RS485.SetEcho_Sleep(echo_sleep_ ? MIDEA_AC_ACTIVE : MIDEA_AC_INACTIVE);
    ESP32_Midea_RS485.SetSwing(swing_ ? MIDEA_AC_ACTIVE : MIDEA_AC_INACTIVE);
    ESP32_Midea_RS485.SetVent(vent_ ? MIDEA_AC_ACTIVE : MIDEA_AC_INACTIVE);
  }

  void publish_ac_state_() {
    if (mode_select_) {
      std::string mode;
      switch (ESP32_Midea_RS485.State.OpMode) {
        case MIDEA_AC_OPMODE_AUTO: mode = "Auto"; break;
        case MIDEA_AC_OPMODE_OFF:  mode = "Off";  break;
        case MIDEA_AC_OPMODE_COOL: mode = "Cool"; break;
        case MIDEA_AC_OPMODE_HEAT: mode = "Heat"; break;
        case MIDEA_AC_OPMODE_DRY:  mode = "Dry";  break;
        case MIDEA_AC_OPMODE_FAN:  mode = "Fan";  break;
        default:                   mode = "Unknown"; break;
      }
      mode_select_->publish_state(mode);
      desired_mode_ = mode;
    }

    if (fan_mode_select_) {
      std::string fan_mode;
      switch (ESP32_Midea_RS485.State.FanMode) {
        case MIDEA_AC_FANMODE_AUTO:   fan_mode = "Auto";   break;
        case MIDEA_AC_FANMODE_HIGH:   fan_mode = "High";   break;
        case MIDEA_AC_FANMODE_MEDIUM: fan_mode = "Medium"; break;
        case MIDEA_AC_FANMODE_LOW:    fan_mode = "Low";    break;
        default:                      fan_mode = "Unknown"; break;
      }
      fan_mode_select_->publish_state(fan_mode);
      desired_fan_mode_ = fan_mode;
    }

    if (temp_number_) {
      float temp = (ESP32_Midea_RS485.State.SetTemp > 0)
                   ? (float)ESP32_Midea_RS485.State.SetTemp
                   : 16.0f;
      temp_number_->publish_state(temp);
      desired_temp_ = (uint8_t)temp;
    }

    if (aux_heat_switch_) {
      bool v = (ESP32_Midea_RS485.State.OperatingFlags & 0x02) > 0;
      aux_heat_switch_->publish_state(v);
      aux_heat_ = v;
    }
    if (echo_sleep_switch_) {
      bool v = (ESP32_Midea_RS485.State.OperatingFlags & 0x01) > 0;
      echo_sleep_switch_->publish_state(v);
      echo_sleep_ = v;
    }
    if (vent_switch_) {
      bool v = (ESP32_Midea_RS485.State.OperatingFlags & 0x88) > 0;
      vent_switch_->publish_state(v);
      vent_ = v;
    }
    if (swing_switch_) {
      bool v = (ESP32_Midea_RS485.State.OperatingFlags & 0x04) > 0;
      swing_switch_->publish_state(v);
      swing_ = v;
    }
  }
};

}  // namespace midea_rs485
}  // namespace esphome
