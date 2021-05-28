#include "ble_ota.h"
#include <WiFi.h>
#include "Arduino.h"
#include "ESP32_BLE_WiFI_Conf.h"
/** Build time */
const char compileDate[] = __DATE__ " " __TIME__;

//Contructor
Esp32BLEWifiConf::Esp32BLEWifiConf()
{

}

//Begin which setup everything
void Esp32BLEWifiConf::begin(uint16_t startdelayms, uint16_t recoverydelayms)
{
  // init:
  // Create unique device name
  createName();

  // Initialize Serial port
  Serial.begin(115200);
  // Send some device info
  Serial.print("Build: ");
  Serial.println(compileDate);

  /* Print chip information */
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  Serial.printf("This is %s chip with %d CPU core(s), WiFi%s%s, ",
                CONFIG_IDF_TARGET,
                chip_info.cores,
                (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
                (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

  Serial.printf("silicon revision %d, ", chip_info.revision);

  Serial.printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
                (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

  Serial.printf("Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());

  // Set Preference
  setPreference();
  // Start BLE server
  initBLE();

  if (getCredentials()) {
    // Check for available AP's
    if (!scanWiFi) {
      Serial.println("Could not find any AP");
    } else {
      // If AP was found, start connection
      connectWiFi();
    }
  }
}

void Esp32BLEWifiConf::process()
{
  if (connStatusChange()) {
    if (isConnect()) {
      Serial.print("Connected to AP: ");
      Serial.print(WiFi.SSID());
      Serial.print(" with IP: ");
      Serial.print(WiFi.localIP());
      Serial.print(" RSSI: ");
      Serial.println(WiFi.RSSI());
	  
	  notifyIP(WiFi.localIP().toString().c_str());
		
    } else {
      if (getCredentials()) {
        Serial.println("Lost WiFi connection");
        // Received WiFi credentials
        if (!scanWiFi) { // Check for available AP's
          Serial.println("Could not find any AP");
        } else { // If AP was found, start connection
          connectWiFi();
        }
      }
    }
    setConnStatus(false);
  }
}

std::string Esp32BLEWifiConf::getBLECommand()
{
	if(isCommandReceived()) {
		std::string command;
		command = getCommand();
		return command;
	}
}

bool Esp32BLEWifiConf::isBLECommand() {
	return isCommandReceived();
}


void Esp32BLEWifiConf::notifyBLE(char* value) {
	notifyOK(value);
}
