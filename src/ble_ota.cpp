#ifndef _BLE_OTA_H_
#include "ble_ota.h"
#endif

// Default Arduino includes
#include <Arduino.h>
#include <WiFi.h>
#include <nvs.h>
#include <nvs_flash.h>

// Includes for JSON object handling
// Requires ArduinoJson library
// https://arduinojson.org
// https://github.com/bblanchon/ArduinoJson
#include <ArduinoJson.h>
#include <string>

// Includes for BLE
#include <BLE2902.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEDevice.h>
#include <BLEAdvertising.h>
#include <Preferences.h>
/** SSIDs of local WiFi networks */
String ssidPrim;
String ssidSec;
/** Password for local WiFi network */
String pwPrim;
String pwSec;
bool hasCredentials;
bool connStatusChanged;
bool isConnected;
bool commandReceived;
/** Unique device name */
char apName[] = "ESP32-xxxxxxxxxxxx";
/** Selected network
    true = use primary network
    false = use secondary network
*/
bool usePrimAP = true;
bool deviceConnected = false;

/**
   Create unique device name from MAC address
 **/

/** Buffer for JSON string */
// MAx size is 51 bytes for frame:
// {"ssidPrim":"","pwPrim":"","ssidSec":"","pwSec":""}
// + 4 x 32 bytes for 2 SSID's and 2 passwords
StaticJsonDocument<256> jsonBuffer;

/** Characteristic for digital output */
BLECharacteristic *pCharacteristicWiFi;
/** Characteristic for IP Notification */
BLECharacteristic *pCharacteristicIP;
/** Characteristic for command */
BLECharacteristic *pCharacteristicCommand;
/** BLE Advertiser */
BLEAdvertising *pAdvertising;
/** BLE Service */
BLEService *pService;
/** BLE Service */
BLEService *pService2;
/** BLE Server */
BLEServer *pServer;

// List of Service and Characteristic UUIDs
#define SERVICE_UUID "0124b940-bfe3-11eb-8529-0242ac130003"
#define WIFI_UUID "0124bb8e-bfe3-11eb-8529-0242ac130003"
#define IP_UUID "0124bc7e-bfe3-11eb-8529-0242ac130003"
#define COMMAND_UUID "0124bd3c-bfe3-11eb-8529-0242ac130003"
#define COMMAND_NOTIFY_UUID "0124c03e-bfe3-11eb-8529-0242ac130003"
using namespace std;

void createName()
{
    uint8_t baseMac[6];
    // Get MAC address for WiFi station
    esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
    // Write unique name into apName
    sprintf(apName, "ESP32-%02X%02X%02X%02X%02X%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
}

void setPreference()
{
    Preferences preferences;
    preferences.begin("WiFiCred", false);
    bool hasPref = preferences.getBool("valid", false);
    if (hasPref)
    {
        ssidPrim = preferences.getString("ssidPrim", "");
        ssidSec = preferences.getString("ssidSec", "");
        pwPrim = preferences.getString("pwPrim", "");
        pwSec = preferences.getString("pwSec", "");

        if (ssidPrim.equals("") || pwPrim.equals("") || ssidSec.equals("") || pwPrim.equals(""))
        {
            Serial.println("Found preferences but credentials are invalid");
        }
        else
        {
            Serial.println("Read from preferences:");
            Serial.println("primary SSID: " + ssidPrim + " password: " + pwPrim);
            Serial.println("secondary SSID: " + ssidSec + " password: " + pwSec);
            hasCredentials = true;
        }
    }
    else
    {
        Serial.println("Could not find preferences, need send data over BLE");
    }
    preferences.end();
}
/**
   MyServerCallbacks
   Callbacks for client connection and disconnection
*/

class MyServerCallbacks : public BLEServerCallbacks
{

    // TODO this doesn't take into account several clients being connected
    void onConnect(BLEServer *pServer)
    {
        deviceConnected = true;
        Serial.println("BLE client connected");
        Serial.println("\n");
        Serial.println("Ready to receive command ...");
        Serial.println("\n");
    };

    void onDisconnect(BLEServer *pServer)
    {
        deviceConnected = false;
        Serial.println("BLE client disconnected");
        pAdvertising->start();
    }
};

bool isCommandReceived()
{
    return commandReceived;
}

bool BLEConnected()
{
    return deviceConnected;
}
/**
   CommandCallbackHandler
   Callbacks for BLE client command requests
*/

class CommandCallbackHandler : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristicCommand)
    {
        std::string value = pCharacteristicCommand->getValue();
        if (value.length() == 0)
        {
            return;
        }
        commandReceived = true;
        //	  Serial.println("Received command over BLE: " + String((char *)&value[0]));
    }
};

/**
   MyCallbackHandler
   Callbacks for BLE client read/write requests
*/

class MyCallbackHandler : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        std::string value = pCharacteristic->getValue();
        if (value.length() == 0)
        {
            return;
        }
        //    Serial.println("Received over BLE: " + String((char *)&value[0]));

        // Decode data
        // int keyIndex = 0;
        //  for (int index = 0; index < value.length(); index++)
        //  {
        //     value[index] = (char)value[index] ^ (char)apName[keyIndex];
        //     keyIndex++;
        //    if (keyIndex >= strlen(apName))
        //        keyIndex = 0;
        //   }
		string conf = XOR(value);
	//	Serial.println(conf.c_str());
        /** Json object for incoming data */
        // JsonObject &jsonIn = jsonBuffer.parseObject((char *)&value[0]);
		
        auto jsonIn = deserializeJson(jsonBuffer, conf);
        if (!jsonIn)
        {
            if (jsonBuffer["ssidPrim"] &&
                jsonBuffer["pwPrim"] &&
                jsonBuffer["ssidSec"] &&
                jsonBuffer["pwSec"])
            {
                ssidPrim = jsonBuffer["ssidPrim"].as<const char *>();
                pwPrim = jsonBuffer["pwPrim"].as<const char *>();
                ssidSec = jsonBuffer["ssidSec"].as<const char *>();
                pwSec = jsonBuffer["pwSec"].as<const char *>();

                Preferences preferences;
                preferences.begin("WiFiCred", false);
                preferences.putString("ssidPrim", ssidPrim);
                preferences.putString("ssidSec", ssidSec);
                preferences.putString("pwPrim", pwPrim);
                preferences.putString("pwSec", pwSec);
                preferences.putBool("valid", true);
                preferences.end();

                Serial.println("Received over bluetooth:");
                Serial.println("primary SSID: " + ssidPrim + " password: " + pwPrim);
                Serial.println("secondary SSID: " + ssidSec + " password: " + pwSec);
                connStatusChanged = true;
                hasCredentials = true;
            }
            else if (jsonBuffer["erase"])
            {
                Serial.println("Received erase command");
                Preferences preferences;
                preferences.begin("WiFiCred", false);
                preferences.clear();
                preferences.end();
                connStatusChanged = true;
                hasCredentials = false;
                ssidPrim = "";
                pwPrim = "";
                ssidSec = "";
                pwSec = "";

                int err;
                err = nvs_flash_init();
                Serial.println("nvs_flash_init: " + err);
                err = nvs_flash_erase();
                Serial.println("nvs_flash_erase: " + err);
            }
            else if (jsonBuffer["reset"])
            {
                WiFi.disconnect();
                esp_restart();
            }
        }
        else
        {
            Serial.println("Received invalid JSON");
        }
        jsonBuffer.clear();
    };

    void onRead(BLECharacteristic *pCharacteristic)
    {
        // Serial.println("BLE onRead request");
        String wifiCredentials;

        /** Json object for outgoing data */

        DynamicJsonDocument jsonOut(1024);

        jsonOut["ssidPrim"] = ssidPrim;
        jsonOut["pwPrim"] = pwPrim;
        jsonOut["ssidSec"] = ssidSec;
        jsonOut["pwSec"] = pwSec;
        // Convert JSON object into a string
        serializeJson(jsonOut, wifiCredentials);

        Serial.println("Stored settings: " + wifiCredentials);
		
        std::string encodedCredentials;
		
        encodedCredentials = XOR(wifiCredentials.c_str());
        pCharacteristicWiFi->setValue(encodedCredentials);
        jsonBuffer.clear();
    }
};

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

void initBLE()
{
    // Initialize BLE and set output power
    BLEDevice::init(apName);
    BLEDevice::setPower(ESP_PWR_LVL_P7);

    // Create BLE Server
    pServer = BLEDevice::createServer();

    // Set server callbacks
    pServer->setCallbacks(new MyServerCallbacks());

    // Create BLE Service

    pService = pServer->createService(SERVICE_UUID);

    // Create BLE Characteristic for WiFi settings
    pCharacteristicWiFi = pService->createCharacteristic(
        WIFI_UUID,
        // WIFI_UUID,
        BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE);

    pCharacteristicWiFi->setCallbacks(new MyCallbackHandler());

    // Create BLE Characteristic for WiFi settings
    pCharacteristicIP = pService->createCharacteristic(
        IP_UUID,
        BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_NOTIFY |
            BLECharacteristic::PROPERTY_INDICATE);
    /// Add desctiptor to characteristic
    pCharacteristicIP->addDescriptor(new BLE2902());

    pService2 = pServer->createService(COMMAND_UUID);

    // Create BLE Characteristic for WiFi settings
    pCharacteristicCommand = pService2->createCharacteristic(
        COMMAND_NOTIFY_UUID,
        BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_NOTIFY |
            BLECharacteristic::PROPERTY_INDICATE);

    pCharacteristicCommand->setCallbacks(new CommandCallbackHandler());
    /// Add desctiptor to characteristic
    pCharacteristicCommand->addDescriptor(new BLE2902());

    // Start the service
    pService->start();

    // Start the service
    pService2->start();

    // Start advertising
    pAdvertising = pServer->getAdvertising();
    pAdvertising->start();
}

void setCredentials(bool credentials)
{
    hasCredentials = credentials;
}

bool getCredentials()
{
    return hasCredentials;
}

void setConnStatus(bool connStatus)
{
    connStatusChanged = connStatus;
}
/** Callback for receiving IP address from AP */
void gotIP(system_event_id_t event)
{
    isConnected = true;
    connStatusChanged = true;
}

bool isConnect()
{
    return isConnected;
}

void notifyIP(std::string addressIP)
{

    pCharacteristicIP->setValue(addressIP);
    pCharacteristicIP->notify();
    // pCharacteristicIP -> indicate();
}

std::string getCommand()
{
    std::string value = pCharacteristicCommand->getValue();
    return value;
}

void notifyOK(char *value)
{
    pCharacteristicCommand->setValue(value);
    pCharacteristicCommand->notify();
    commandReceived = false;
}

/** Callback for connection loss */
void lostCon(system_event_id_t event)
{
    isConnected = false;
    connStatusChanged = true;
}

bool connStatusChange()
{
    return connStatusChanged;
}

/**
   scanWiFi
   Scans for available networks
   and decides if a switch between
   allowed networks makes sense

   @return <code>bool</code>
          True if at least one allowed network was found
*/

bool scanWiFi()
{
    /** RSSI for primary network */
    int8_t rssiPrim;
    /** RSSI for secondary network */
    int8_t rssiSec;
    /** Result of this function */
    bool result = false;

    Serial.println("Start scanning for networks");

    WiFi.disconnect(true);
    WiFi.enableSTA(true);
    WiFi.mode(WIFI_STA);

    // Scan for AP
    int apNum = WiFi.scanNetworks(false, true, false, 1000);
    if (apNum == 0)
    {
        Serial.println("Found no networks?????");
        return false;
    }

    byte foundAP = 0;
    bool foundPrim = false;

    for (int index = 0; index < apNum; index++)
    {
        String ssid = WiFi.SSID(index);
        Serial.println("Found AP: " + ssid + " RSSI: " + WiFi.RSSI(index));
        if (!strcmp((const char *)&ssid[0], (const char *)&ssidPrim[0]))
        {
            Serial.println("Found primary AP");
            foundAP++;
            foundPrim = true;
            rssiPrim = WiFi.RSSI(index);
        }
        if (!strcmp((const char *)&ssid[0], (const char *)&ssidSec[0]))
        {
            Serial.println("Found secondary AP");
            foundAP++;
            rssiSec = WiFi.RSSI(index);
        }
    }

    switch (foundAP)
    {
    case 0:
        result = false;
        break;
    case 1:
        if (foundPrim)
        {
            usePrimAP = true;
        }
        else
        {
            usePrimAP = false;
        }
        result = true;
        break;
    default:
        Serial.printf("RSSI Prim: %d Sec: %d\n", rssiPrim, rssiSec);
        if (rssiPrim > rssiSec)
        {
            usePrimAP = true; // RSSI of primary network is better
        }
        else
        {
            usePrimAP = false; // RSSI of secondary network is better
        }
        result = true;
        break;
    }
    return result;
}

/**
   Start connection to AP
*/

void connectWiFi()
{
    // Setup callback function for successful connection
    WiFi.onEvent(gotIP, SYSTEM_EVENT_STA_GOT_IP);
    // Setup callback function for lost connection
    WiFi.onEvent(lostCon, SYSTEM_EVENT_STA_DISCONNECTED);

    WiFi.disconnect(true);
    WiFi.enableSTA(true);
    WiFi.mode(WIFI_STA);

    Serial.println();
    Serial.print("Start connection to ");
    if (usePrimAP)
    {
        Serial.println(ssidPrim);
        WiFi.begin(ssidPrim.c_str(), pwPrim.c_str());
    }
    else
    {
        Serial.println(ssidSec);
        WiFi.begin(ssidSec.c_str(), pwSec.c_str());
    }
}
