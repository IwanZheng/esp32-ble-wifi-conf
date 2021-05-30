# esp32-ble-wifi-conf
Using BLE to set WiFi configuration on ESP32 boards

Sometimes we just don't want to hardcode our SSID and password into hardware.

This is one of the solution.
Using BLE (Bluetooth Low Energy) capability of ESP32, we can set WiFi SSID and password after flashing the hardware.

This code uses Preferences.h, where we can input up to two SSID at one time.
And will choose between SSID depending on which one has a greater availibility.

This code itself is very simple and uses simple XOR encryption to transmit and receive data.


Usage example:
Simplest way to use this code.

```cpp
#include "src/ESP32_BLE_WiFi_Conf.h";

//global variable
Esp32BLEWifiConf bleconf;

void setup()
{
  bleconf.begin();
}

void loop() {
  bleconf.process();
}
```

Other example:

We are getting command from BLE;
and acknowledge by sending a string back.

```cpp
#include "src/ESP32_BLE_WiFi_Conf.h";

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
```

Encryption provided is minimal. 
Simple strings XOR with number 1 to obfuscate data transmission.

NOTE: Please alter the code to avoid any security issue.

```cpp
std::string XOR(std::string wifiCredential)
{
    // encode the data
    int keyIndex = 1;
    std::string encodedCredential;

    for (int index = 0; index < wifiCredential.length(); index++)
    {
        encodedCredential += (char)wifiCredential[index] ^ keyIndex;
    }

    return encodedCredential;
};
```

Provide yourself a decoding algorithm on your software.

Some rudimentary software is provided at https://esp32pwa.web.app to check for features.

Happy coding!

Cheers.