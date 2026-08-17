#include "ESP32_Samsung.h"
#include "HardwareSerial.h"

namespace esphome {

ESP32_Samsung_Class::ESP32_Samsung_Class() {
//re_de_pin(re_de_pin),
//di_pin(di_pin),
//ro_pin(ro_pin),
//slaveId(kSamsungSlaveId),
//masterId(kSamsungMasterId) {
}

void ESP32_Samsung_Class::begin(int ro_pin, int di_pin, int re_de_pin) {


  Serial.begin(9600);
  Serial.write("hello");
  Serial2.begin(2400, SERIAL_8E1, ro_pin, di_pin);
  //pinMode(re_de_pin, OUTPUT);
  //digitalWrite(re_de_pin, HIGH);
}

void ESP32_Samsung_Class::on() {
    this->onOff = (uint8_t)kSamsungPowerOn;
}

void ESP32_Samsung_Class::off() {
    this->onOff = (uint8_t)kSamsungPowerOff;
}

void ESP32_Samsung_Class::setFan(uint8_t f) {
    this->fan = f;
}

void ESP32_Samsung_Class::setMode(uint8_t m) {
    this->mode = m;
}

void ESP32_Samsung_Class::setTemp(uint8_t t) {
    this->temp = t;
}

uint8_t ESP32_Samsung_Class::checkSum(uint8_t *msg, int len) {
  int csum = 0;

  for (int i = 0; i < len; i++) {
    char c = msg[i];
    csum = csum ^ c;
  }
  return csum;
}

#define FAN_HIGH  6
#define MODE_COOL 1
#define MODE_HEAT 4
#define MODE_OFF  0xc4
#define MODE_ON   0xf4


void ESP32_Samsung_Class::send() {

    uint8_t coolModeOn[] = { 0x85, 0x20, 0xa0, 0x1a, 0x18, (FAN_HIGH << 5) + 18, 1, MODE_ON, 0, 0, 0};

    ESP_LOGD("samsung", "send()...");

    tx_data[0] =  (uint8_t)kSamsungPreamble;
    tx_data[1] =  (uint8_t)kSamsungSlaveId;
    tx_data[2] =  (uint8_t)kSamsungMasterId;
    tx_data[3] =  (uint8_t)kSamsungCmdSettings;
    tx_data[4] =  (uint8_t)kSamsungSwingOn;
    tx_data[5] =  0x18; // Why 0x18 in the other?
    tx_data[6] =  (uint8_t)(fan) + (uint8_t)temp;
    tx_data[7] =  (uint8_t)mode;
    tx_data[8] =  (uint8_t)onOff;
    tx_data[9] =  0;
    tx_data[10] = 0;
    tx_data[11] = 0;
    tx_data[12] = checkSum(&tx_data[1], kSamsungMessageLen);
    tx_data[13] = (uint8_t)kSamsungPrologue;
    Serial2.write(&tx_data[0], 14);
    delay(20);
    Serial2.write(&tx_data[0], 14);
    delay(20);
    Serial2.write(&tx_data[0], 14);
    for (int i = 0; i < 14; i++) {
	ESP_LOGD(TAG, "data 0x%02hhx", tx_data[i]);
    }
    //Serial1.write(0x32);
    //Serial1.write(&coolModeOn[0], 11);
    //Serial1.write(checkSum(&coolModeOn[0], 11));
    //Serial1.write(0x34);
}

ESP32_Samsung_Class ESP32_Samsung;
}
