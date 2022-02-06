/*
 */

#include "Arduino.h"

#include "VUEF.h"
#include "CLI.h"
#include "Command.h"
#include "ConfigReg.h"

#include <string.h>

static const char OK_TEXT[] PROGMEM = "OK";

const char* help_text = FST("\
help                                   Print help\r\n\
?                                      Print help\r\n\
\r\n");

char serial_command[COMMAND_BUFFER_SIZE];
size_t serial_command_index = 0;
CommandLineInterpreter serial_cli = CommandLineInterpreter(&Serial);

void ReadSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    serial_command[serial_command_index++] = c;
    if (c == '\n' || c == '\r') { 
      serial_command[serial_command_index-1] = '\0';
      serial_command_index = 0;
      DEBUG_print(F("Execute: "));
      DEBUG_println(serial_command);
      DEBUG_println(serial_cli.execute(serial_command));
    }
    if (serial_command_index > COMMAND_BUFFER_SIZE-1) {
      serial_command_index = COMMAND_BUFFER_SIZE-1;
    }
  }
}

const char* CommandLineInterpreter::execute(const char* cmd) {
  error = nullptr;
  size_t n = 0; 
  if (*cmd == '#') { return FST(""); } // Comment line
  if ((n = StrTool::tryRead(FST("HELP"), cmd))) { return help(); }
  if ((n = StrTool::tryRead(FST("?"), cmd))) { return help(); }

  RegVar* var = RegGroup::mainGroup->findVarByFullName(cmd, false);
  if (var) {
    while (*cmd && *cmd != ' ') { cmd++; };
    while (*cmd == ' ') { cmd++; }
    if (*cmd) {
      const char* errorStr = nullptr;
      var->setFromStr(cmd, &errorStr);
      if (errorStr) { setError(errorStr); return errorStr; }
      return OK_TEXT;
    } else {
      var->toStr(buffer, sizeof(buffer));
      return buffer;
    }
  }

  Command* sCmd = CommandRegistry::mainCmdReg->findCmdByFullName(cmd, false);
  if (sCmd) {
    while (*cmd && *cmd != ' ') { cmd++; };
    while (*cmd == ' ') { cmd++; }
    ErrorCode err = sCmd->execute(cmd, stream);
    if (err) { return FST(""); }
    return OK_TEXT;
  }

  return setError(FST("invalid command"));
}

const char* CommandLineInterpreter::help() {
  if (stream) {
    stream->println();
    //for(auto i : *RegGroup::mainGroup) {
    for (RegGroup::Iterator i = RegGroup::mainGroup->begin(), end = RegGroup::mainGroup->end(); i != end; i++) {
      RegVar& v = *i;
      if (v.isHidden()) { continue; }
      char name[64];
      i.getVarName(name, sizeof(name));
      sprintf(buffer, FST("%s [%s]"), name, v.typeHelp());
      stream->printf(FST("%-38s Get/Set %s\r\n"), buffer, v.info());
    }

    stream->println();
    for (CommandRegistry::Iterator i = CommandRegistry::mainCmdReg->begin(), end = CommandRegistry::mainCmdReg->end(); i != end; i++) {
      Command& c = *i;
      if (c.isHidden()) { continue; }
      char name[64];
      i.getCmdName(name, sizeof(name));
      sprintf(buffer, FST("%s %s"), name, c.inputHelp());
      stream->printf(FST("%-38s %s\r\n"), buffer, c.info());
    }

    stream->println();
    stream->println(help_text);
  }
  return OK_TEXT;  
}
