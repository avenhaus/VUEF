#include <VUEF.h>
#include "SPIFFS.h"
#include "ConfigReg.h"
#include "StateReg.h"


const char HOSTNAME[] PROGMEM = __HOSTNAME__;
const char PROJECT_NAME[] PROGMEM = __PROJECT_NAME__;
const char PROJECT_VERSION[] PROGMEM = __PROJECT_COMPILE__;
const char COMPILE_DATE[] PROGMEM = __DATE__;
const char COMPILE_TIME[] PROGMEM = __TIME__;

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
