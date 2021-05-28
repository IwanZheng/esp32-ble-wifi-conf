#include "src/ESP32_BLE_WiFI_Conf.h";

//global variable
Esp32BLEWifiConf bleconf;

void setup()
{
  bleconf.begin();
}

void loop() {
  bleconf.process();
  if(bleconf.isBLECommand()) {
     std::string command = bleconf.getBLECommand();
     // Do something with command
     Serial.print(command.c_str());
     Serial.print('\n');
     bleconf.notifyBLE("OK");
  }
}
