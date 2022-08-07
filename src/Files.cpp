#include <Arduino.h>
#include <SPIFFS.h>
#include <set>

#include "VUEF.h"
#include "Helper.h"
#include "Command.h"
#include "Files.h"

#if ENABLE_SPIFFS

static const char* ROOT_DIR PROGMEM = "/";

// Behavior of SPIFFS name() has changed
// #define FULL_NAME name
#define FULL_NAME path

/*----------------------------------------------------------------------*\
| SPIFFS command registry
\*----------------------------------------------------------------------*/

#if ENABLE_CLI
CommandRegistry cmdRegSpiffs(FST("spiffs"));

Command cmdSpiffsDir(FST("dir"), 
[] (const char* args, Print* stream) {
    ErrorCode ec = spiffsFiles(args, *stream);
    stream->println();
    return ec;
},
FST("Get files on SPIFFS as JSON"), &cmdRegSpiffs, FST("[path]")
);

Command cmdSpiffsJDir(FST("jdir"), 
[] (const char* args, Print* stream) {
    ErrorCode ec = spiffsFilesJson(args, *stream);
    stream->println();
    return ec;
},
FST("Get files on SPIFFS as JSON"), &cmdRegSpiffs, FST("[path]"), CT_APP_JSON
);

Command cmdSpiffsDelete(FST("rm"), 
[] (const char* args, Print* stream) {
    ErrorCode ec = spiffsDeleteFile(args, stream);
    stream->println();
    return ec;
},
FST("Delete file from SPIFFS"), &cmdRegSpiffs, FST("<path>")
);

Command cmdSpiffsDelDir(FST("rmdir"), 
[] (const char* args, Print* stream) {
    ErrorCode ec = spiffsDeleteDir(args, stream);
    stream->println();
    return ec;
},
FST("Delete directory from SPIFFS"), &cmdRegSpiffs, FST("<path>")
);

Command cmdSpiffsCreateDir(FST("mkdir"), 
[] (const char* args, Print* stream) {
    ErrorCode ec = spiffsCreateDir(args, stream);
    stream->println();
    return ec;
},
FST("Create directory in SPIFFS"), &cmdRegSpiffs, FST("<path>")
);

Command cmdSpiffsRead(FST("read"), 
[] (const char* args, Print* stream) {
    ErrorCode ec = spiffsRead(args, *stream);
    stream->println();
    return ec;
},
FST("Read file from SPIFFS"), &cmdRegSpiffs, FST("<path>")
);

Command cmdSpiffsWrite(FST("write"), 
[] (const char* args, Print* stream) {
    if (!args || !*args) {
        if (stream) { stream->println(FST("no path")); }
        return EC_BAD_REQUEST;
    }
    const char* data = args;
    while (*data && *data != ' ') { data++; }
    char path[128];
    size_t n = std::min(sizeof(path)-1, (size_t)(data-args));
    memcpy(path, args, n);
    path[n] = '\0';
    while (*data && *data == ' ') { data++; }
    size_t size = strlen(data);
    ErrorCode ec = spiffsWrite(path, data, size, stream);
    stream->println();
    return ec;
},
FST("Write file to SPIFFS"), &cmdRegSpiffs, FST("<path> [data]")
);


Command cmdSpiffsRename(FST("mv"), 
[] (const char* args, Print* stream) {
    if (!args || !*args) {
        if (stream) { stream->println(FST("no path")); }
        return EC_BAD_REQUEST;
    }
    const char* newPath = args;
    while (*newPath && *newPath != ' ') { newPath++; }
    char oldPath[128];
    size_t n = std::min(sizeof(oldPath)-1, (size_t)(newPath-args));
    memcpy(oldPath, args, n);
    oldPath[n] = '\0';
    while (*newPath && *newPath == ' ') { newPath++; }
    ErrorCode ec = spiffsRename(oldPath, newPath, stream);
    stream->println();
    return ec;
},
FST("Rename file on SPIFFS"), &cmdRegSpiffs, FST("<old> <new>")
);


Command cmdSpiffsRenameDir(FST("mvdir"), 
[] (const char* args, Print* stream) {
    if (!args || !*args) {
        if (stream) { stream->println(FST("no path")); }
        return EC_BAD_REQUEST;
    }
    const char* newPath = args;
    while (*newPath && *newPath != ' ') { newPath++; }
    char oldPath[128];
    size_t n = std::min(sizeof(oldPath)-1, (size_t)(newPath-args));
    memcpy(oldPath, args, n);
    oldPath[n] = '\0';
    while (*newPath && *newPath == ' ') { newPath++; }
    ErrorCode ec = spiffsRenameDir(oldPath, newPath, stream);
    stream->println();
    return ec;
},
FST("Rename directory on SPIFFS"), &cmdRegSpiffs, FST("<old> <new>")
);

#endif // ENABLE_CLI

/*----------------------------------------------------------------------*\
| SPIFFS functions
\*----------------------------------------------------------------------*/

ErrorCode spiffsDeleteFile(const char* path, Print* s) {
  if (!SPIFFS.exists(path)) {
      if(s) { s->print(path); s->print(FST(" does not exists!")); }
      return EC_NOT_FOUND;
  } else if (!SPIFFS.remove(path)) {
      if(s) { s->print(path); s->print(FST(" failed to delete")); }
      return EC_ERROR;
  }
  if(s) { s->print(path); s->print(FST(" deleted")); }

  String ptmp = path;
  int  nstart = ptmp.lastIndexOf('/');
  if (nstart > 1) { ptmp = ptmp.substring(0, nstart); }

  // On SPIFFS no files means no directory
  if ((ptmp != ROOT_DIR) && (ptmp[ptmp.length() - 1] == '/')) {
      ptmp = ptmp.substring(0, ptmp.length() - 1);
  }

  File dir = SPIFFS.open(ptmp);
  File dircontent = dir.openNextFile();
  if (!dircontent) {
      // Keep empty directory with help of dummy file
      File r = SPIFFS.open(ptmp + FST("/."), FILE_WRITE);
      if (r) {
          r.close();
      }
  }
  return EC_OK;
}

ErrorCode spiffsDeleteDir(const char* path, Print* s) {
  if (!strcmp(path, ROOT_DIR)) {
    if(s) { s->print(FST("can not delete root")); }
    return EC_BAD_REQUEST;
  }

  ErrorCode ec = EC_OK;
  File dir = SPIFFS.open(path);
  {
      File file = dir.openNextFile();
      while (file) {
          const char* fullpath = file.FULL_NAME();
          if (!SPIFFS.remove(fullpath)) {
              ec = EC_ERROR;
              if(s) { s->print(fullpath); s->print(FST(" failed to delete")); }
          }
          file = dir.openNextFile();
      }
  }
  if (!ec && s) { s->print(path); s->print(FST(" deleted")); }
  return ec;
}

ErrorCode spiffsRenameDir(const char* oldPath, const char* newPath, Print* s) {
    if (!oldPath || !*oldPath) {
        if (s) { s->print(FST("no old path")); }
        return EC_BAD_REQUEST;
    }
    if (!newPath || !*newPath) {
        if (s) { s->print(FST("no new path")); }
        return EC_BAD_REQUEST;
    }
    if (!strcmp(oldPath, ROOT_DIR)) {
        if(s) { s->print(FST("can not rename root")); }
        return EC_BAD_REQUEST;
    }

    size_t oldLen = strlen(oldPath);
    if (oldPath[oldLen-1] != '/') { oldLen++; }
    char newName[128];
    size_t newLen = 0;

    File dir = SPIFFS.open(oldPath);
    if (!dir) {
        if (s) { s->print(oldPath); s->print(FST(" does not exist!")); }
        return EC_BAD_REQUEST;
    } 

    while (newPath[newLen] && newLen < sizeof(newName)-3) { newName[newLen] = newPath[newLen]; newLen++; }
    if (newName[newLen-1] != '/') { newName[newLen++] = '/'; }
    ErrorCode ec = EC_OK;
    {
        File file = dir.openNextFile();
        while (file) {
            const char* oldName = file.FULL_NAME();
            size_t n = oldLen;
            size_t m = newLen;
            while (newName[n] && m < sizeof(newName)-2) { newName[m++] = oldName[n++]; }
            newName[m] = '\0';
            DEBUG_printf(FST("Rename %s to %s\n"), oldName, newName);
            if (!SPIFFS.rename(oldName, newName)) {
                if (s) { s->print(oldName); s->print(FST(" failed to rename to ")); s->print(newName); }
                return EC_ERROR;
            }
            file = dir.openNextFile();
        }
    }
    if (!ec && s) { s->print(oldPath); s->print(FST(" renamed")); }
    return ec;
}

ErrorCode spiffsCreateDir(const char* path, Print* s) {
  String dummyFile = path;
  dummyFile += FST("/.");
  if (SPIFFS.exists(dummyFile)) {
    if (s) { s->print(path); s->print(FST(" already exists!")); }
    return EC_BAD_REQUEST;
  } 

  File r = SPIFFS.open(dummyFile, FILE_WRITE);
  if (!r) {
    if (s) { s->print(path); s->print(FST(" could not be created!")); }
    return EC_ERROR;
  } 
  r.close();
  if (s) { s->print(path); s->print(FST(" created")); }
  return EC_OK;
}

ErrorCode spiffsRead(const char* path, Print& s) {
    File file = SPIFFS.open(path);
    if (!file) {
        s.print(path); 
        s.print(FST(" failed to open!"));
        return EC_ERROR;
    } 
    while (file.available()) {
        s.write(file.read());
    }
    file.close();
    return EC_OK;
}

ErrorCode spiffsWrite(const char* path, const char* data, size_t size, Print* s) {
    if (!path || !*path) {
        if (s) { s->print(FST("no path")); }
        return EC_BAD_REQUEST;
    }
    if (SPIFFS.exists(path)) {
        if (s) { s->print(path); s->print(FST(" already exists!")); }
        return EC_BAD_REQUEST;
    } 
    File file = SPIFFS.open(path, FILE_WRITE);
    if (!file) {
        if (s) { s->print(path); s->print(FST(" could not be created!")); }
        return EC_ERROR;
    } 
    file.write((const uint8_t*) data, size);
    file.close();
    if (s) { s->print(path); s->printf(FST(" created and %d bytes written\r\n"), size); }
    return EC_OK;
}

ErrorCode spiffsRename(const char* oldPath, const char* newPath, Print* s) {
    if (!oldPath || !*oldPath) {
        if (s) { s->print(FST("no old path")); }
        return EC_BAD_REQUEST;
    }
    if (!newPath || !*newPath) {
        if (s) { s->print(FST("no new path")); }
        return EC_BAD_REQUEST;
    }
    if (!SPIFFS.exists(oldPath)) {
        if (s) { s->print(oldPath); s->print(FST(" does not exist!")); }
        return EC_BAD_REQUEST;
    } 
    if (SPIFFS.exists(newPath)) {
        if (s) { s->print(newPath); s->print(FST(" already exists!")); }
        return EC_BAD_REQUEST;
    }
    if (!SPIFFS.rename(oldPath, newPath)) {
        if (s) { s->print(oldPath); s->print(FST(" failed to rename!")); }
        return EC_ERROR;
    }
    if (s) { s->print(oldPath); s->print(FST(" renamed to ")); s->print(newPath); }
    return EC_OK;    
    }


ErrorCode spiffsFiles(const char* path, Print& s) {
    if (!path || !path[0]) { path = ROOT_DIR; }
    char buffer[64];

    File dir = SPIFFS.open(path);
    if (!dir) {
        s.print(path); s.println(FST(" failed to open!"));
        return EC_ERROR;
    }
    File currentFile = dir.openNextFile();
    while (currentFile) {
        StrTool::formatBytes(buffer, sizeof(buffer), currentFile.size());
        s.printf(FST("%-12s  %s\r\n"), buffer, currentFile.FULL_NAME());
        currentFile = dir.openNextFile();
    }
    dir.close();
    size_t totalBytes = SPIFFS.totalBytes();
    StrTool::formatBytes(buffer, sizeof(buffer), totalBytes);
    s.print(FST("\r\n  Size: ")); s.print(buffer);
    size_t usedBytes = SPIFFS.usedBytes();
    StrTool::formatBytes(buffer, sizeof(buffer), usedBytes);
    s.printf(FST("  |  Used: %s (%d%%)\r\n"),  buffer, 100 * usedBytes / totalBytes);
    return EC_OK;
}

ErrorCode spiffsFilesJson(const char* path, Print& s, const char* status/*=nullptr*/) {
    if (!path || !path[0]) { path = ROOT_DIR; }
    size_t pathLen = strlen(path);
    if (pathLen>1) { pathLen++; }
    char buffer[64];
    s.write('{');

    File dir = SPIFFS.open(path);
    if (!dir) {
        s.print(FST("\"error\":\"failed to open\",\"file\":\""));
        s.print(path); s.print(FST("\"}")); 
        return EC_ERROR;
    }
    s.write(FST("\"files\":["));
    bool isFirst = true;
    std::set<String> subDirs;
    File currentFile = dir.openNextFile();
    while (currentFile) {
        String filename = currentFile.FULL_NAME();
        filename = filename.substring(pathLen, filename.length());
        bool isVisible = true;
        // Check for sub dir
        int pathEnd = filename.indexOf('/');
        if (pathEnd != -1) {
            filename = filename.substring(0, pathEnd);
            if (filename.length() == 0 || subDirs.find(filename) != subDirs.end()) {
                isVisible = false; // We already have this dir
            } else {
                buffer[0] = '-'; buffer[1] = '1'; buffer[2] = '\0'; // -1 means dir
                subDirs.insert(filename);
            }
        } else {
            //do not add "." file
            if (!((filename == FST(".")) || (filename == EMPTY_STRING))) {
                StrTool::formatBytes(buffer, sizeof(buffer), currentFile.size());
            } else {
                isVisible = false;
            }
        }
        if (isVisible) {
            if (!isFirst) { s.write(','); }
            else { isFirst = false; }
            s.write('{');
            s.print(FST("\"name\":\""));
            s.print(filename);
            s.print(FST("\",\"size\":\""));
            s.print(buffer);
            s.write('"');
            s.write('}');
        }
        currentFile = dir.openNextFile();
    }
    dir.close();
    s.print(FST("],\"path\":\""));
    s.print(path);
    s.print(FST("\""));
    if (status && *status) {
      s.print(FST(",\"status\":\""));
      s.print(status);
      s.write('"');
    }
    s.print(FST(",\"total\":\""));
    size_t totalBytes = SPIFFS.totalBytes();
    size_t usedBytes = SPIFFS.usedBytes();
    StrTool::formatBytes(buffer, sizeof(buffer), totalBytes);
    s.print(buffer);
    s.print(FST("\",\"used\":\""));
    StrTool::formatBytes(buffer, sizeof(buffer), usedBytes);
    s.print(buffer);
    s.printf(FST("\",\"occupation\":%d"), 100 * usedBytes / totalBytes);
    s.print(FST("}"));
    return EC_OK;
}

#endif // ENABLE_SPIFFS
