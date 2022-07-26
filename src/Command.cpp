/*======================================================================*\
|* Command Registry
|*
|* This framework provides a hierarchical registry for commands. 
\*======================================================================*/

#include <WiFi.h>
#include <EEPROM.h>

#include "VUEF.h"
#include "Command.h"
#include "Helper.h"

#if !ENABLE_WIFI || !ENABLE_WEB_SERVER
const char CT_TEXT_PLAIN[] PROGMEM = "text/plain";
const char CT_APP_JSON[] PROGMEM = "application/json";
#endif

#if ENABLE_CLI
CommandRegistry* CommandRegistry::mainCmdReg = nullptr;

/************************************************************************\
|* Global Functions
\************************************************************************/


/************************************************************************\
|* Command Registry
\************************************************************************/

CommandRegistry* CommandRegistry::findChild(const char* name) {
    for(auto g: children_) {
      if (StrTool::matchesCleanName(name, g->name())) { return g; }
    }
    return nullptr;
}

Command* CommandRegistry::findCmd(const char* name) {
    for(auto v: cmds_) {
      if (StrTool::matchesCleanName(name, v->name())) { return v; }
    }
    return nullptr;
}

Command* CommandRegistry::findCmdByFullName(const char* name, bool matchCase/*=true*/) {
    size_t n = 0;
    for(auto v: cmds_) {
      n = StrTool::matchesNamePart(name, v->name(), matchCase);
      if (n && name[n] != '.') { return v; }
    }
    for(auto g: children_) {
      n = StrTool::matchesNamePart(name, g->name(), matchCase);
      if (n && name[n] == '.') { return g->findCmdByFullName(name + n + 1, matchCase); }
    }
    return nullptr;
}

Command* CommandRegistry::get(size_t n) {
    if (n >= cmdCount_) { return nullptr; }
    if (n < cmds_.size()) { return cmds_.at(n); }
    n -= cmds_.size();
    for(auto g: children_) {
      if (n < g->size()) { return g->get(n); }
      n -= g->size();
    }
    return nullptr;
}

size_t CommandRegistry::getCmdName(char* buffer, size_t size, size_t index) {

    if (index >= cmdCount_) { return 0; }
    if (index < cmds_.size()) { 
      return StrTool::toCleanName(buffer, size, cmds_.at(index)->name()); 
    }
    index -= cmds_.size();
    for(auto g: children_) {
      if (index < g->size()) { 
        size_t n = StrTool::toCleanName(buffer, size, g->name_);
        if (n < size -1) { buffer[n++] = '.'; }
        buffer[n] = '\0';
        return g->getCmdName(buffer+n, size-n, index); 
      }
      index -= g->size();
    }
    return 0;
}

std::vector<Command*>::iterator CommandRegistry::getIt(size_t n) {
    if (n >= cmdCount_) { return cmds_.end(); }
    if (n <= cmds_.size()) { return cmds_.begin() + n; }
    n -= cmds_.size();
    for(auto g: children_) {
      if (n <= g->size()) { return g->getIt(n); }
      n -= g->size();
    }
    return cmds_.end();
}

#include "Network.h"
#include <driver/uart.h>

/************************************************************************\
|* Controller Commands
\************************************************************************/

Command cmdRestart(FST("restart"), 
[] (const char* args, Print* stream) {
    ESP.restart();
    return EC_OK;
},
FST("Restart controller")
);

Command cmdDebug(FST("debug"), 
[] (const char* args, Print* stream) {
  bool state = false;
  if ( args[0] == '\0' ) {
    if (debugStream == nullptr) { 
      stream->print(FST("off"));
      return EC_OK;
    } else if (debugStream == &Serial) {
      stream->print(FST("serial"));
      return EC_OK;
    } else if (debugStream == stream) {
      stream->print(FST("here"));
      return EC_OK;
    } else {
      stream->print(FST("on"));
      return EC_OK;
    }
  }
  const char* errorStr = nullptr;
  StrTool::readBool(args, &state, &errorStr);
  if (errorStr) { stream->print(errorStr); return EC_BAD_ARGUMENT; }
  debugStream = state ? stream : nullptr;
  return EC_OK;
},
FST("Set debug mode"),
nullptr, FST("[bool]")
);

#include <SPIFFS.h>
Command cmdStats(FST("stats"), 
[] (const char* args, Print* stream) {
    char buffer[64];
    stream->printf(FST("Chip ID: %d\r\n"), (uint16_t)(ESP.getEfuseMac() >> 32));
    stream->printf(FST("CPU Frequency: %d MHz\r\n"), ESP.getCpuFreqMHz());
    stream->printf(FST("CPU Temperature: %.2f C\r\n"), temperatureRead());
    StrTool::formatBytes(buffer, sizeof(buffer), ESP.getFreeHeap());
    stream->printf(FST("Free memory: %s\r\n"), buffer);
    StrTool::formatDurationUs(buffer, sizeof(buffer), esp_timer_get_time());
    stream->printf(FST("Up Time: %s\r\n"), buffer);
    #if ENABLE_NTP && ENABLE_WIFI
    char fmt[48];
    size_t n = snprintf(fmt, sizeof(fmt)-1, FST("%s | %s"), configTimeFormat.get(), configDateFormat.get());
    fmt[n] = '\0';
    getTimeStr(buffer, fmt);
    #endif
    stream->printf(FST("Time: %s\r\n"), buffer);
    stream->printf(FST("SDK: %s\r\n"), ESP.getSdkVersion());
    StrTool::formatBytes(buffer, sizeof(buffer), ESP.getFlashChipSize());
    stream->printf(FST("Flash Size: %s\r\n"), buffer);
#if ENABLE_SPIFFS
    size_t totalBytes = SPIFFS.totalBytes();
    size_t usedBytes = SPIFFS.usedBytes();
    StrTool::formatBytes(buffer, sizeof(buffer), totalBytes);
    stream->print(FST("SPIFFS Size: ")); stream->println(buffer);
    StrTool::formatBytes(buffer, sizeof(buffer), usedBytes);
    stream->printf(FST("SPIFFS Used: %s (%d%%)\r\n"), buffer, 100 * usedBytes / totalBytes);
#endif // ENABLE_SPIFFS
#if ENABLE_WIFI
    wifiInfo(*stream);
#endif
    uint32_t baud = 0;
    uart_get_baudrate(UART_NUM_0, &baud);
    stream->printf(FST("Baud rate: %d\r\n"), (baud / 100) * 100);
    stream->printf(FST("FW version: %s %s\r\n"), PROJECT_NAME, PROJECT_VERSION);
    stream->printf(FST("FW compiled: %s %s\r\n"), COMPILE_DATE, COMPILE_TIME);
    return EC_OK;
},
FST("Show controller stats")
);

#endif //ENABLE_CLI