#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>
#include "ConfigReg.h"

void networkInit(void);
void networkRun(void);

bool wifiWaitForStaConnect();
bool wifiStartSTA();
bool wifiStartAP();
bool wifiStart();
void wifiReset();
void wifiStop();
bool wifiServicesStart();
void wifiServicesStop();

String getWifiMac();
size_t getWifiId(char* buffer, size_t bSize=1<<30);
size_t WifiGetJsonInfo(char* buffer);
IPAddress getLocalIp();
int getRSSI();
void WifiSleep();
void WifiWake();
void getNtpTime();
time_t getEpochTime();
uint32_t getMillisDelay(uint32_t frequency);
size_t getIsoTime(char* buffer);
size_t getTimeStr(char* buffer, const char* fmt);
ErrorCode listWifiNetworksJson(Print& out, bool filterEmpty=true);

void printMac(Print& s, uint8_t *mac);
void wifiInfo(Print& s);

#if ENABLE_NTP
extern ConfigStr configTimeZone;
extern ConfigStr configTimeFormat;
extern ConfigStr configDateFormat;
#endif

extern ConfigStr configHostname;
extern char fullHostname[];

#endif // NETWORK_H