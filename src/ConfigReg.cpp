/*======================================================================*\
|* Configuration Variable Registry
|*
|* This framework provides a hierarchical registry for configuration 
|* variables. Each variable can register defaults, formatting, references,
|* getter/setter callbacks etc.
|*
|* The variables can be persisted in EEPROM (as JSON). The registry
|* can be used to control or read variables from CLI, Web Interfaces
|* or Display GUI interfaces.
\*======================================================================*/

#include <WiFi.h>
#include <EEPROM.h>

#include "VUEF.h"
#include "ConfigReg.h"
#include "Helper.h"
#include "Command.h"


RegGroup* RegGroup::mainGroup = nullptr;
size_t RegVar::count_ = 0;

bool isConfigOk = false;


/************************************************************************\
|* Config Commands
\************************************************************************/

#if ENABLE_CLI
CommandRegistry cmdRegConfig(FST("config"));

extern char fullHostname[];
Command cmdBasicConfig(FST("basics"), 
[] (const char* args, Print* stream) {
    #if ENABLE_SPIFFS
      #define SPIFFS_LOC "/sd"
    #else
      #define SPIFFS_LOC "No SD"
    #endif // ENABLE_SPIFFS
    #if ENABLE_WIFI
    stream->printf(FST("FW name: %s # FW version: %s # primary sd:" SPIFFS_LOC " # secondary sd:No SD # authentication:no # webcommunication: Sync: /ws # hostname:%s # tabs: " WEBUI_TABS " # start: "),
    PROJECT_NAME, PROJECT_VERSION, fullHostname);
    #else
    stream->printf(FST("FW name: %s # FW version: %s # primary sd:" SPIFFS_LOC " # secondary sd:No SD # authentication:no # webcommunication: Sync: /ws # tabs: " WEBUI_TABS " # start: "),
    PROJECT_NAME, PROJECT_VERSION);
    #endif
    const char* startTab = FST(WEBUI_START_TAB);
    #if ENABLE_WEB_UI_WIZARD
      if (!isConfigOk) { startTab = FST("wizard"); }
    #endif
    stream->println(startTab);
    return EC_OK;
},
FST("Get basic configuration"), &cmdRegConfig
);

Command cmdSaveConfig(FST("save"), 
[] (const char* args, Print* stream) {
    saveConfig();
    return EC_OK;
},
FST("Save configuration to EEPROM"), &cmdRegConfig
);

Command cmdLoadConfig(FST("load"), 
[] (const char* args, Print* stream) {
    loadConfig();
    return EC_OK;
},
FST("Load configuration from EEPROM"), &cmdRegConfig
);

Command cmdDefaultConfig(FST("def"), 
[] (const char* args, Print* stream) {
    defaultConfig();
    return EC_OK;
},
FST("Set default configuration"), &cmdRegConfig
);

Command cmdGetConfig(FST("get"), 
[] (const char* args, Print* stream) {
    RegGroup::mainGroup->toJson(stream, true, RF_IS_CONFIG, RF_IS_CONFIG);
    stream->println();
    return EC_OK;
},
FST("Get configuration as JSON"), &cmdRegConfig,
nullptr, CT_APP_JSON
);

Command cmdConfigVar(FST("var"), 
[] (const char* args, Print* stream) {
    RegVar* var = RegGroup::mainGroup->findVarByFullName(args, false);
    if (var) {
        while (*args && *args != ' ') { args++; };
        while (*args == ' ') { args++; }
        if (*args) {
            const char* errorStr = nullptr;
            var->setFromStr(args, &errorStr);
            if (stream && errorStr) { stream->print(errorStr); }
            return errorStr ? EC_BAD_REQUEST : EC_OK;
        } else {
            if (stream) { var->print(*stream); }
        }
    } else {
         if (stream) { stream->print(FST("unknown variable")); }
         return EC_NOT_FOUND;
    }
    if (stream) { stream->println(); }
    return EC_OK;
},
FST("Get or set configuration variable"), &cmdRegConfig, FST("<name> [value]"), CT_APP_JSON
);

Command cmdGetConfigUi(FST("ui"), 
[] (const char* args, Print* stream) {
    RegGroup::mainGroup->getWebUi(stream, true, RF_IS_CONFIG, RF_IS_CONFIG);
    stream->println();
    return EC_OK;
},
FST("Get configuration UI as JSON"), &cmdRegConfig,
nullptr, CT_APP_JSON
);

#if ENABLE_WEB_UI_WIZARD 
Command cmdGetWizardUi(FST("wizard-ui"), 
[] (const char* args, Print* stream) {
    RegGroup::mainGroup->getWebUi(stream, true, RF_IS_CONFIG | RF_WIZARD, RF_IS_CONFIG | RF_WIZARD);
    stream->println();
    return EC_OK;
},
FST("Get wizard UI as JSON"), &cmdRegConfig,
nullptr, CT_APP_JSON
);
#endif // ENABLE_WEB_UI_WIZARD 

Command cmdGetControlUi(FST("control-ui"), 
[] (const char* args, Print* stream) {
    RegGroup::mainGroup->getWebUi(stream, true, RF_CONTROL_UI, RF_CONTROL_UI);
    stream->println();
    return EC_OK;
},
FST("Get control UI as JSON"), &cmdRegConfig,
nullptr, CT_APP_JSON
);

#endif // ENABLE_CLI

/************************************************************************\
|* Global Functions
\************************************************************************/

void saveConfig() {
  char buffer[CONFIG_BUFFER_SIZE];
  size_t size = RegGroup::mainGroup->toJsonStr(buffer, sizeof(buffer), true, RF_SHOW_PASSWORD | RF_IS_CONFIG, RF_NOT_PERSISTED | RF_IS_CONFIG);
  if (size >= CONFIG_BUFFER_SIZE-1) {
    DEBUG_printf(FST("!!! Not enough buffer space (%d) to save config to EEPROM !!!\n"), CONFIG_BUFFER_SIZE);
    return;
  }
  uint32_t crc = StrTool::calculateCrc(buffer, size);
  DEBUG_printf(FST("Saving config of size %d to EEPROM. Buffer space:%d CRC:%0X\n"), size, CONFIG_BUFFER_SIZE, crc);

  ConfigHeader header = {
    CONFIG_MAGIC,
    size,
    crc
  };

  EEPROM.begin(sizeof(ConfigHeader) + size);
  for (size_t i=0; i<sizeof(ConfigHeader); i++) { EEPROM.write(i, ((uint8_t*)&header)[i]); }
  for (size_t i=0; i<size; i++) { EEPROM.write(i+sizeof(ConfigHeader), buffer[i]); }
  EEPROM.end();
  yield();
  DEBUG_println(F("Wrote config data to EEPROM"));
}

bool loadConfig() {
  ConfigHeader header;
  char buffer[CONFIG_BUFFER_SIZE];
  EEPROM.begin(sizeof(ConfigHeader) + CONFIG_BUFFER_SIZE);
  bool ok = true;
  for (int i=0; i<sizeof(ConfigHeader); i++) { ((uint8_t*)&header)[i] = EEPROM.read(i); }
  if (header.magic != CONFIG_MAGIC) {
    DEBUG_print(F("Bad EEPROM magic: ")); 
    DEBUG_println(header.magic, HEX); 
    ok = false;
  }
  DEBUG_printf(FST("EEPROM config size: %d  Buffer Size: %d\n"), header.size, CONFIG_BUFFER_SIZE); 
  if (header.size > CONFIG_BUFFER_SIZE-1) {
    DEBUG_print(F("Bad EEPROM config size: ")); 
    DEBUG_println(header.size); 
    ok = false;
  }
  if (ok) {
    for (int i=0; i<header.size; i++) { buffer[i] = EEPROM.read(i+sizeof(ConfigHeader)); }
    buffer[header.size] = '\0';
    uint32_t crc = StrTool::calculateCrc(buffer, header.size);
    if (crc != header.crc) {
      DEBUG_printf(FST("Bad EEPROM data crc: %0X vs header crc: %0X\n"), crc, header.crc); 
      ok = false;
    }
    if (ok) {
      DEBUG_println(F("Found valid EEPROM config data.")); 
      DEBUG_println(buffer);
    }
  }
  EEPROM.end();
  yield();
  if (ok) { ok |= !parseConfigJson(buffer); }
  isConfigOk = ok;
  return ok;
}

bool parseConfigJson(char* jsonStr) {
  StaticJsonDocument<CONFIG_BUFFER_SIZE> doc; // !!!!! TODO:: check best size !!!!!!
  DeserializationError err = deserializeJson(doc, jsonStr);
  if (err != DeserializationError::Ok) {
    DEBUG_printf(FST("JSON config parsing failed: %s\n"), (const char *)err.f_str());
    return true;
  }
  JsonObject obj = doc.as<JsonObject>();
  return RegGroup::mainGroup->setFromJson(obj);
}


void defaultConfig() {
  if (RegGroup::mainGroup) { RegGroup::mainGroup->setDefaults(); }
}


/************************************************************************\
|* Registration Group
\************************************************************************/

size_t RegGroup::toJsonStr(char* buffer, size_t size, bool noName, uint32_t flags/*=0*/, RFFlag flagsMask/*=0*/) {
    size_t n = 0;
    if (!noName) { n += StrTool::toJsonName(buffer+n, size-n, name_); }
    if (n < size-1) { buffer[n++] = '{'; }
    bool first = true;
    for(auto v: vars_) {
      if (!((v->flags() ^ flags) & flagsMask)) {
        if (!first && (n < size-1)) { buffer[n++] = ','; }
        first = false; 
        n += v->toJsonStr(buffer+n, size-n, false, flags, flagsMask);
      }
    }
    for(auto g: children_) {
      if (!((g->flags() ^ flags) & flagsMask)) {
        if (!first && (n < size-1)) { buffer[n++] = ','; }
        first = false; 
        n += g->toJsonStr(buffer+n, size-n, false, flags, flagsMask);
      }
    }
    if (n < size-1) { buffer[n++] = '}'; }
    buffer[n] = '\0';
    return n;
}

size_t RegGroup::toJson(Print* stream, bool noName, uint32_t flags/*=0*/, RFFlag flagsMask/*=0*/) {
    char buffer[128];
    size_t size = sizeof(buffer);
    size_t n = 0;
    if (!noName) { n += StrTool::toJsonName(buffer, size, name_); }
    if (n < size-1) { buffer[n++] = '{'; }
    stream->write(buffer, n);
    bool first = true;
    for(auto v: vars_) {
      if (!((v->flags() ^ flags) & flagsMask)) {
        if (!first) { stream->write(','); n++; }
        first = false; 
        size_t m = v->toJsonStr(buffer, size, false, flags, flagsMask);
        stream->write(buffer, m);
        n += m;
      }
    }
    for(auto g: children_) {
      if (!((g->flags() ^ flags) & flagsMask)) {
        if (!first) { stream->write(','); n++; }
        first = false; 
        n += g->toJson(stream, false, flags, flagsMask);
      }
    }
    stream->write('}'); n++;
    return n;
}

size_t RegGroup::getWebUi(Print* stream, bool noName, uint32_t flags/*=0*/, RFFlag flagsMask/*=0*/) {
    char buffer[128];
    size_t size = sizeof(buffer);
    size_t n = 0;
    if (!noName) { 
      n += StrTool::toJsonName(buffer, size, name_); 
      stream->write(buffer, n);
    }
    n += stream->print(FST("{\"_INFO_\":{"));
    n += stream->printf(FST("\"L\":\"%s\""), name_);
    if (info_) {n += stream->printf(FST(",\"H\":\"%s\""), info_); }
    if (flags_) { n += stream->printf(FST(",\"F\":%d"), flags_); }
    bool first = true;
    n += stream->print(FST("},\"_VARS_\":["));
    for(auto v: vars_) {
      if (!v->isHidden() && !((v->flags() ^ flags) & flagsMask) ) {
        if (!first) { stream->write(','); n++; }
        first = false;
        size_t m = v->getWebUi(stream, flags, flagsMask);
        n += m;
      }
    }
    stream->write(']'); n++;
    for(auto g: children_) {
      if (!((g->flags() ^ flags) & (flagsMask & (RF_IS_CONFIG | RF_IS_STATE))) && g->getVarCount(flags, flagsMask)) {
        { stream->write(','); n++; }
        first = false; 
        n += g->getWebUi(stream, false, flags, flagsMask);
      }
    }
    stream->write('}'); n++;
    return n;
}

size_t RegGroup::getVarCount(uint32_t flags/*=0*/, RFFlag flagsMask/*=0*/) {
    size_t n = 0;
    for(auto v: vars_) {
      if (!v->isHidden() && !((v->flags() ^ flags) & flagsMask) ) { n++; }
    }
    for(auto g: children_) {
      if (!((g->flags() ^ flags) & (flagsMask & (RF_IS_CONFIG | RF_IS_STATE)))) {
        n += g->getVarCount(flags, flagsMask);
      }
    }
    return n;
}


RegGroup* RegGroup::findChild(const char* name) {
    for(auto g: children_) {
      if (StrTool::matchesCleanName(name, g->name())) { return g; }
    }
    return nullptr;
}

RegVar* RegGroup::findVar(const char* name) {
    for(auto v: vars_) {
      if (StrTool::matchesCleanName(name, v->name())) { return v; }
    }
    return nullptr;
}

RegVar* RegGroup::findVarByFullName(const char* name, bool matchCase/*=true*/) {
    size_t n = 0;
    for(auto v: vars_) {
      n = StrTool::matchesNamePart(name, v->name(), matchCase);
      if (n && name[n] != '.') { return v; }
    }
    for(auto g: children_) {
      n = StrTool::matchesNamePart(name, g->name(), matchCase);
      if (n && name[n] == '.') { return g->findVarByFullName(name + n + 1, matchCase); }
    }
    return nullptr;
}

RegVar* RegGroup::get(size_t n) {
    if (n >= size()) { return nullptr; }
    if (n < vars_.size()) { return vars_.at(n); }
    n -= vars_.size();
    for(auto g: children_) {
      if (n < g->size()) { return g->get(n); }
      n -= g->size();
    }
    return nullptr;
}

size_t RegGroup::getVarName(char* buffer, size_t bSize, size_t index) {
    if (index >= size()) { return 0; }
    if (index < vars_.size()) { 
      return StrTool::toCleanName(buffer, bSize, vars_.at(index)->name()); 
    }
    index -= vars_.size();
    for(auto g: children_) {
      if (index < g->size()) { 
        size_t n = StrTool::toCleanName(buffer, bSize, g->name_);
        if (n < bSize -1) { buffer[n++] = '.'; }
        buffer[n] = '\0';
        return g->getVarName(buffer+n, bSize-n, index); 
      }
      index -= g->size();
    }
    return 0;
}


std::vector<RegVar*>::iterator RegGroup::getIt(size_t n) {
    if (n >= size()) { return vars_.end(); }
    if (n <= vars_.size()) { return vars_.begin() + n; }
    n -= vars_.size();
    for(auto g: children_) {
      if (n <= g->size()) { return g->getIt(n); }
      n -= g->size();
    }
    return vars_.end();
}


bool RegGroup::setFromJson(const JsonObject& obj) {
  bool err = false;
  // Loop through all the key-value pairs in obj
  for (JsonPair p : obj) {
    RegGroup* child = findChild(p.key().c_str());
    if (child) { err |= child->setFromJson(p.value()); }
    else {
      RegVar* var = findVar(p.key().c_str());
      if (var) { err |= var->setFromJson(p.value()); }
      else { 
        DEBUG_printf(FST("Could not find JSON config \"%s\"\n"), p.key().c_str());
        err = true; 
      }
    }
  }
  return err;
}

void RegGroup::setDefaults() {
    for(auto v: vars_) { v->setDefault(); }
    for(auto g: children_) { g->setDefaults(); }
}

void RegGroup::addVar(RegVar* var) { 
  vars_.push_back(var);
  if (var->flags() & RF_IS_CONFIG) { updateConfigCount_(1); }
  if (var->flags() & RF_IS_STATE) { updateStateCount_(1); }
}

void RegGroup::removeVar(RegVar* var) { 
  size_t tmp=vars_.size(); 
  vars_.erase(remove(vars_.begin(), vars_.end(), var), vars_.end()); 
  if (var->flags() & RF_IS_CONFIG) { updateConfigCount_(vars_.size() - tmp); }
  if (var->flags() & RF_IS_STATE) { updateStateCount_(vars_.size() - tmp); }
}
