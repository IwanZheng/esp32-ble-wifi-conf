
#include <string>

#ifndef ESP32_BLE_WIFI_CONF_H
  #define ESP32_BLE_WIFI_CONF_H
  //be sure correct IDE and settings are used for ESP8266 or ESP32
  #if !(defined(ARDUINO_ARCH_ESP32))
  #error Oops!  Make sure you have 'ESP32' compatible board selected from the 'Tools -> Boards' menu.
  #endif

 
  class Esp32BLEWifiConf
  {
    public:
      Esp32BLEWifiConf();
      void begin(uint16_t startdelayms = 8000, uint16_t recoverydelayms = 8000);
      void process();
	  std::string getBLECommand();
	  bool isBLECommand();
	  void notifyBLE(char* value);
  };
#endif
