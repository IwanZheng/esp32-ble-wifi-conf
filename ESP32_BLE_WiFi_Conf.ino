#include "src/ESP32_BLE_WiFI_Conf.h";
//global variable
Esp32BLEWifiConf bleconf;

void setup()
{
  bleconf.begin();
}

void loop() {
  bleconf.process();
}
