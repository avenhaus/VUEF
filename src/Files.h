#ifndef _SPIFFS_CMD_H_
#define _SPIFFS_CMD_H_

#include <Arduino.h>
#include "VUEF.h"

#if ENABLE_SPIFFS

ErrorCode spiffsFiles(const char* path, Print& s);
ErrorCode spiffsFilesJson(const char* path, Print& s, const char* status=nullptr);
ErrorCode spiffsDeleteFile(const char* path, Print* s);
ErrorCode spiffsDeleteDir(const char* path, Print* s);
ErrorCode spiffsCreateDir(const char* path, Print* s);
ErrorCode spiffsRead(const char* path, Print& s);
ErrorCode spiffsWrite(const char* path, const char* data, size_t size, Print* s);
ErrorCode spiffsRename(const char* oldPath, const char* newPath, Print* s);
ErrorCode spiffsRenameDir(const char* oldPath, const char* newPath, Print* s);

#endif // ENABLE_SPIFFS
#endif // _SPIFFS_CMD_H_