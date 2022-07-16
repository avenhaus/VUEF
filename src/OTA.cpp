#include <Arduino.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include "VUEF.h"
#if ENABLE_WIFI

void otaInit(const char* hostname) {

    DEBUG_println(FST("Enabling OTA"));
    ArduinoOTA.setHostname(hostname);

    // Port defaults to 3232
    // ArduinoOTA.setPort(3232); 

    // No authentication by default
    // ArduinoOTA.setPassword("admin");

    // Password can be set with it's md5 value as well
    // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
    // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

    ArduinoOTA.onStart([]() {
        //NOTE: make .detach() here for all functions called by Ticker.h library - not to interrupt transfer process in any way.
        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        DEBUG_printf(FST("Start updating %s\n"), ArduinoOTA.getCommand() == U_FLASH ? FST("sketch") : FST("file system"));
    });
  
    ArduinoOTA.onEnd([]() {
        DEBUG_println(FST("\nOTA End"));
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        DEBUG_printf(FST("Progress: %u%%\r"), (progress / (total / 100)));
    });
  
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) { DEBUG_println(FST("\nAuth Failed")); }
        else if (error == OTA_BEGIN_ERROR) { DEBUG_println(FST("\nBegin Failed")); }
        else if (error == OTA_CONNECT_ERROR) { DEBUG_println(FST("\nConnect Failed")); }
        else if (error == OTA_RECEIVE_ERROR) { DEBUG_println(FST("\nReceive Failed")); }
        else if (error == OTA_END_ERROR) { DEBUG_println(FST("\nEnd Failed")); }
    });

    ArduinoOTA.begin();

    DEBUG_printf(FST("OTA initialized. Hostname: '%s' IP address: "), hostname);
    DEBUG_println(getLocalIp());
}
#endif // ENABLE_WIFI
