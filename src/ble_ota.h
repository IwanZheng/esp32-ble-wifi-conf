#ifndef _BLE_OTA_H_
#define _BLE_OTA_H_ 1

#include <string>

bool BLEConnected();

void setPreference();

void notifyIP(std::string addressIP);

void notifyOK(char*);

bool isCommandReceived();

void setCredentials(bool credentials);

std::string getCommand();

bool getCredentials();

bool isConnect();

void setConnStatus(bool connStatus);

bool connStatusChange();
  
void createName();

void initBLE();

extern bool scanWiFi();

extern void connectWiFi();

#endif
