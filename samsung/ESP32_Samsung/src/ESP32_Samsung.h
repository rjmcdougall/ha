/*
    ESP32 library to control/monitor Samsung air conditioner via RS485 bus
*/


#ifndef _esp32_samsung_h
#define _esp32_samsung_h


    
#include "Arduino.h"

namespace esphome {

#define kSamsungMinTemp 15
#define kSamsungMaxTemp 25

#define kSamsungBaudRate     2400

#define kSamsungRoPin        17 
#define kSamsungDiPin        16
#define kSamsungReDePin      4
#define kSamsungMasterId     0x20
#define kSamsungSlaveId      0x85

#define kSamsungPreamble     0x32
#define kSamsungPrologue     0x34

#define kSamsungSwingOn      0x1a
#define kSamsungSwingOff     0x1f
    
#define kSamsungCmdSettings  0xA0
#define kSamsungCmdStatus1   0x52
#define kSamsungCmdQuery2    0x53
#define kSamsungCmdQuery3    0x54
    
#define kSamsungFanAuto      0x00
#define kSamsungFanLow       (0x02 << 5)
#define kSamsungFanMedium    (0x04 << 5)
#define kSamsungFanHigh      (0x05 << 5)
    
#define kSamsungModeAuto     0x00
#define kSamsungModeCool     0x01
#define kSamsungModeDry      0x02
#define kSamsungModeFan      0x03
#define kSamsungModeHeat     0x04
    
#define kSamsungPowerOff     0xC4
#define kSamsungPowerOn      0xF4

#define kSamsungMessageLen   11
    
class ESP32_Samsung_Class {
  public:
    ESP32_Samsung_Class();
    virtual void begin(int ro_pin, int di_pin, int re_de_pin);
    virtual void on();
    virtual void off();
    virtual void send();
    void setMode(uint8_t mode);
    void setFan(uint8_t fan_mode);
    void setTemp(uint8_t temp);
    uint8_t tx_data[16];
    uint8_t rx_data[128];
  private:
    int ro_pin;
    int di_pin;
    int re_de_pin;
    int slaveId;
    int masterId;
    uint8_t mode;
    uint8_t fan;
    uint8_t temp;
    uint8_t onOff;
  protected:
    uint8_t checkSum(uint8_t *data, int len);
};

extern ESP32_Samsung_Class ESP32_Samsung;  
}
#endif