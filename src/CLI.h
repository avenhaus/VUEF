/*

 */

#ifndef CLI_H
#define CLI_H

#include <Stream.h>

void ReadSerial();

class CommandLineInterpreter {

public:
  CommandLineInterpreter(Stream* stream=nullptr) : stream(stream) { }
  const char* execute(const char* cmd);
  inline const char* setError(const char* text) { error = text; return text; }

protected:
  const char* help();

  Stream* stream;
  char buffer[512];
  const char* error;
};


#endif // CLI_H
