#include <VUEF.h>
#include "SPIFFS.h"
#include "ConfigReg.h"
#include "StateReg.h"


PROGMEM const char* EMPTY_STRING =  "";
PROGMEM const char* NEW_LINE =  "\n";

PROGMEM const char HOSTNAME[] = __HOSTNAME__;
PROGMEM const char PROJECT_NAME[] = __PROJECT_NAME__;
PROGMEM const char PROJECT_VERSION[] = __PROJECT_COMPILE__;
PROGMEM const char COMPILE_DATE[] = __DATE__;
PROGMEM const char COMPILE_TIME[] = __TIME__;

Print* debugStream = &Serial;



void vuefInit() {

  Serial.begin(SERIAL_SPEED);
  DEBUG_printf(FST("\n\n%s %s | %s | %s\n"), PROJECT_NAME, PROJECT_VERSION, COMPILE_DATE, COMPILE_TIME);
  // DEBUG_printf(FST("Compiled with ESP32 SDK:%s\n\n"), ESP.getSdkVersion());

  loadConfig();

  #if ENABLE_WIFI
  #if ENABLE_WEB_SERVER
  if(!SPIFFS.begin(true)){
    DEBUG_println(FST("An Error has occurred while mounting SPIFFS"));
    return;
  }
  #endif

  networkInit();
  #endif // ENABLE_WIFI
}


void vuefRun(uint32_t now/*=0*/) {
  if (now == 0) { now = millis(); }

#if ENABLE_WIFI
#if !USE_NETWORK_TASK
  networkRun();
#endif
#endif // ENABLE_WIFI

#if ENABLE_STATE_REG
  stateRegRun(now);
#endif
}
