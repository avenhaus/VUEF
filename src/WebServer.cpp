
#include <SPIFFS.h>

#include "VUEF.h"
#if ENABLE_WIFI
#include "WebServer.h"
#include "Command.h"
#include "StateReg.h"
#include "Files.h"
#include "Helper.h"
#include <Update.h>

// Default Pages
#if __has_include("DefaultHtmlPages.h")
  #include "DefaultHtmlPages.h"
#else
#ifndef PAGE_NOT_FOUND_TEXT
#define PAGE_NOT_FOUND_TEXT \
    "<HTML>\n<HEAD>\n<title>Redirecting...</title> \n</HEAD>\n<BODY>\n<CENTER>Unknown page : $QUERY$- you will be " \
    "redirected...\n<BR><BR>\nif not redirected, <a href='http://$WEB_ADDRESS$'>click here</a>\n<BR><BR>\n<PROGRESS name='prg' " \
    "id='prg'></PROGRESS>\n\n<script>\nvar i = 0; \nvar x = document.getElementById(\"prg\"); \nx.max=5; \nvar " \
    "interval=setInterval(function(){\ni=i+1; \nvar x = document.getElementById(\"prg\"); \nx.value=i; \nif (i>5) " \
    "\n{\nclearInterval(interval);\nwindow.location.href='/';\n}\n},1000);\n</script>\n</CENTER>\n</BODY>\n</HTML>\n\n"
#endif
const char WebServer::PAGE_NOT_FOUND[] PROGMEM = PAGE_NOT_FOUND_TEXT;

#ifndef PAGE_CAPTIVE_TEXT
#define PAGE_CAPTIVE_TEXT \
    "<HTML>\n<HEAD>\n<title>Captive Portal</title> \n</HEAD>\n<BODY>\n<CENTER>Captive Portal page : $QUERY$- you will be " \
    "redirected...\n<BR><BR>\nif not redirected, <a href='http://$WEB_ADDRESS$'>click here</a>\n<BR><BR>\n<PROGRESS name='prg' " \
    "id='prg'></PROGRESS>\n\n<script>\nvar i = 0; \nvar x = document.getElementById(\"prg\"); \nx.max=5; \nvar " \
    "interval=setInterval(function(){\ni=i+1; \nvar x = document.getElementById(\"prg\"); \nx.value=i; \nif (i>5) " \
    "\n{\nclearInterval(interval);\nwindow.location.href='/';\n}\n},1000);\n</script>\n</CENTER>\n</BODY>\n</HTML>\n\n" 
#endif
const char WebServer::PAGE_CAPTIVE[] PROGMEM = PAGE_CAPTIVE_TEXT;
#endif // __has_include("DefaultHtmlPages.h")

#if __has_include("Favicon.h")
  #include "Favicon.h"
#else
#ifndef FAVICON_BINARY
// Created with GIMP (export compressed .ico) and: xxd -i ESP.ico
#define FAVICON_BINARY { \
  0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x10, 0x10, 0x10, 0x00, 0x01, 0x00, \
  0x04, 0x00, 0xb3, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x89, 0x50, \
  0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, \
  0x44, 0x52, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x10, 0x08, 0x06, \
  0x00, 0x00, 0x00, 0x1f, 0xf3, 0xff, 0x61, 0x00, 0x00, 0x00, 0x7a, 0x49, \
  0x44, 0x41, 0x54, 0x38, 0x8d, 0xc5, 0x53, 0xc9, 0x0d, 0x80, 0x40, 0x08, \
  0x04, 0x62, 0x25, 0x5a, 0x88, 0x5f, 0x1f, 0x6a, 0xaf, 0x6a, 0x1b, 0xb6, \
  0x60, 0x2d, 0xe3, 0x0b, 0x83, 0x08, 0xd1, 0x84, 0x87, 0xf3, 0x59, 0x32, \
  0xcb, 0x31, 0xb3, 0x61, 0x19, 0x00, 0xa8, 0x00, 0xa9, 0x14, 0x13, 0x11, \
  0x71, 0x3f, 0xec, 0x25, 0x05, 0x5c, 0xb5, 0xd0, 0x68, 0xd0, 0xad, 0xd3, \
  0x45, 0x1e, 0xf3, 0x16, 0x72, 0x51, 0x8e, 0xd8, 0x44, 0x25, 0x33, 0x2e, \
  0x6c, 0x0e, 0x00, 0xed, 0x32, 0xc2, 0xe3, 0x8d, 0xd3, 0x58, 0xec, 0x44, \
  0x2b, 0x31, 0xe3, 0xbc, 0x35, 0xf9, 0x22, 0xdd, 0x16, 0x3c, 0xee, 0x33, \
  0xb9, 0x91, 0xf4, 0x28, 0x4f, 0x7c, 0xe7, 0xec, 0xcc, 0x70, 0xb3, 0x10, \
  0xbd, 0x81, 0xb7, 0xe3, 0x51, 0x5e, 0xa4, 0xff, 0x57, 0xb9, 0xfc, 0x1b, \
  0x4f, 0x7c, 0x0c, 0xb2, 0x4d, 0x4a, 0x83, 0x1b, 0x6d, 0x00, 0x00, 0x00, \
  0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82 }
#endif
const unsigned char FAVICON[] PROGMEM = FAVICON_BINARY;
const size_t FAVICON_SIZE = sizeof(FAVICON);
#endif // __has_include("Favicon.h")

const char FAVICON_PATH[] PROGMEM = "/favicon.ico";

#if ENABLE_EMBEDDED_WEB_UI
#include "WebUI.h"
#endif

// HTTP Headers
const char HDR_CONTENT_TYPE[] PROGMEM = "Content-Type";
const char HDR_CONTENT_ENCODING[] PROGMEM = "Content-Encoding";
const char HDR_CE_GZIP[] PROGMEM = "gzip";
const char HDR_CACHE_CONTROL[] PROGMEM = "Cache-Control";
const char HDR_CC_NO_CACHE[] PROGMEM = "no-cache";

const char CT_TEXT_PLAIN[] PROGMEM = "text/plain";
const char CT_TEXT_HTML[] PROGMEM = "text/html";
const char CT_TEXT_CSS[] PROGMEM = "text/css";
const char CT_TEXT_XML[] PROGMEM = "text/xml";
const char CT_IMAGE_PNG[] PROGMEM = "image/png";
const char CT_IMAGE_GIF[] PROGMEM = "image/gif";
const char CT_IMAGE_JPG[] PROGMEM = "image/jpeg";
const char CT_IMAGE_ICON[] PROGMEM = "image/x-icon";
const char CT_APP_YAML[] PROGMEM = "application/x-yaml";
const char CT_APP_JSON[] PROGMEM = "application/json";
const char CT_APP_JS[] PROGMEM = "application/javascript";
const char CT_APP_PDF[] PROGMEM = "application/x-pdf";
const char CT_APP_ZIP[] PROGMEM = "application/x-zip";
const char CT_APP_GZ[] PROGMEM = "application/x-gzip";
const char CT_APP_OCT[] PROGMEM = "application/octet-stream";

const ExtToCt EXT_TO_CT[] PROGMEM = {
  {"htm", CT_TEXT_HTML},
  {"html", CT_TEXT_HTML},
  {"txt", CT_TEXT_PLAIN},
  {"css", CT_TEXT_CSS},
  {"js", CT_APP_JS},
  {"yml", CT_APP_YAML},
  {"yaml", CT_APP_YAML},
  {"json", CT_APP_JSON},
  {"png", CT_IMAGE_PNG},
  {"gif", CT_IMAGE_GIF},
  {"jpeg", CT_IMAGE_JPG},
  {"jpg", CT_IMAGE_JPG},
  {"ico", CT_IMAGE_ICON},
  {"xml", CT_TEXT_XML},
  {"pdf", CT_APP_PDF},
  {"zip", CT_APP_ZIP},
  {"gz", CT_APP_GZ},
  {"", nullptr},
};


const char KEY_IP[] PROGMEM = "$WEB_ADDRESS$";
const char KEY_QUERY[] PROGMEM = "$QUERY$";

extern WebServer* webServer;

const char* getContentType(const char* path) {
  const char* ext = strrchr(path, '.');
  if (!ext) { return CT_APP_OCT; }
  ext++;
  if (*ext == '\0') { return CT_APP_OCT; }
  const ExtToCt* p = EXT_TO_CT;
  while (p->ext[0]) {
    if (!strcasecmp(ext, p->ext)) { return p->ct; }
    p++;
  }
  return CT_APP_OCT;
}

void WebServer::run() {
  
    /*
    if (isReady() && ws.availableForWriteAll()) {
      char buffer[WS_BUFFER_SIZE];

      if (millis() > next_update) {
        nextWsUpdate = millis() + 200;
        //getNews(buffer, sizeof(buffer), &oldState);
        ws.textAll(buffer);
      }
    }
    */
}

void WebServer::handleRoot(AsyncWebServerRequest* request) {
  #if ENABLE_SPIFFS_WEB_UI
  if (trySendFile(request, FST("/index.html"))) { return; }
  if (trySendFile(request, FST("/index.htm"))) { return; }
  #endif

  #if ENABLE_EMBEDDED_WEB_UI
    DEBUG_println(FST("Using embedded WebUI"));
    AsyncWebServerResponse* response = request->beginResponse_P(200, CT_TEXT_HTML, INDEX_HTML_GZ, INDEX_HTML_GZ_SIZE);
    response->addHeader(HDR_CONTENT_ENCODING, HDR_CE_GZIP);
    request->send(response);
    return;
  #else
    return handleNotFound(request);
  #endif
}
  
#if ENABLE_WS_STATE_MONITOR && ENABLE_STATE_REG
void WebServer::wsStateChangeCallback(RegGroup& group, void* data) {
  ((WebServer*)data)->wsStateChangeCallback_(group);
}

void WebServer::wsStateChangeCallback_(RegGroup& group) {
  if (isReady() && ws.availableForWriteAll()) {
    char namePrefix[64];
    char buffer[WS_BUFFER_SIZE];
    PrintBuffer pb(buffer, sizeof(buffer));
    pb.print(FST("UPDATE:"));
    group.printChangeJson(pb, namePrefix, sizeof(namePrefix));
    // DEBUG_println(buffer);
    ws.textAll(buffer);
  }
}
#endif

void WebServer::onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
               AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    webServer->wsState = Connected;
    DEBUG_printf(FST("ws[%s][%u] connect\n"), server->url(), client->id());
    client->printf(FST("CURRENT_ID: %u"), client->id());
    client->printf(FST("ACTIVE_ID: %u"), client->id());
    client->ping();
  } else if (type == WS_EVT_DISCONNECT) {
    webServer->wsState = NoConnection;
    DEBUG_printf(FST("ws[%s][%u] disconnect\n"), server->url(), client->id());
  } else if (type == WS_EVT_ERROR) {
    DEBUG_printf(FST("ws[%s][%u] error(%u): %s\n"), server->url(), client->id(),
        *((uint16_t*)arg), (char*)data);
  } else if (type == WS_EVT_PONG) {
    DEBUG_printf(FST("ws[%s][%u] pong[%u]: %s\n"), server->url(), client->id(), len,
        (len) ? (char*)data : "");
      webServer->wsState = Ready;
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    String msg = "";
    if (info->final && info->index == 0 && info->len == len) {
      // the whole message is in a single frame and we got all of it's data
      DEBUG_printf(FST("ws[%s][%u] %s-msg[%llu]\n"), server->url(), client->id(),
          (info->opcode == WS_TEXT) ? "txt" : "bin", info->len);

      if (info->opcode == WS_TEXT) {
        for (size_t i = 0; i < info->len; i++) {
          msg += (char)data[i];
        }
        DEBUG_printf(FST("%s\n\n"), msg.c_str());

        StaticJsonDocument<100> jsonDocRx;
        deserializeJson(jsonDocRx, msg);

        /*
        if (jsonDocRx.containsKey(F("led"))) {
            uint8_t ledState = jsonDocRx[F("led")];
            if (ledState == 1) {
            digitalWrite(LED_PIN, HIGH);
            }
            if (ledState == 0) {
            digitalWrite(LED_PIN, LOW);
            }
        }
        */

        if (jsonDocRx.containsKey(F("cmd"))) {
            const char* cmd = jsonDocRx[F("cmd")];
            //const char* result = webServer->cmdli.execute(cmd);
        }

        jsonDocRx.clear();
      }
    }
  }
}

void WebServer::handleWebCommand_(AsyncWebServerRequest* request, bool silent) {
    String cmdStr = "";
    static const char* plain = FST("plain");
    static const char* commandText = FST("commandText");
    if (request->hasArg(plain)) {
        cmdStr = request->arg(plain);
    } else if (request->hasArg(commandText)) {
        cmdStr = request->arg(commandText);
    } else {
        DEBUG_println(FST("Invalid Web Command"));
        request->send(400, CT_TEXT_PLAIN, FST("Invalid command"));
        return;
    }
    DEBUG_println(cmdStr);

    const char* cmd = cmdStr.c_str();
    Command* sCmd = CommandRegistry::mainCmdReg->findCmdByFullName(cmd, false);
    if (!sCmd) {
      DEBUG_printf(FST("Invalid Web Command: %s\n"), cmd);
      request->send(404, CT_TEXT_PLAIN, FST("Unknown command"));
      return;
    }
    while (*cmd && *cmd != ' ') { cmd++; };
    while (*cmd == ' ') { cmd++; }
    AsyncResponseStream* response = request->beginResponseStream(sCmd->contentType());
    ErrorCode ec = sCmd->execute(cmd, response);
    if (!ec) { ec = EC_HTTP_OK; }
    response->setCode((int)(ec < 200 ? EC_BAD_REQUEST : ec));
    request->send(response);
    /*
    AsyncResponseStream* response = request->beginResponseStream(CT_TEXT_PLAIN);
    handleCommand(cmd.c_str(), response);
    request->send(response);

    AsyncWebServerResponse* response = request->beginResponse(SPIFFS, pathWithGz, );
    response->printf("dd");
    response->addHeader(HDR_CONTENT_ENCODING, HDR_CE_GZIP);
   request->send(response);

    if (cmd[0]=='[' && cmd[1] = 'E')
    request->send(200, CT_TEXT_PLAIN, FST("FW version:1.1f # FW target:grbl-embedded # FW HW:Direct SD # primary sd:/sd # secondary sd:none # authentication:no # webcommunication: Sync: /ws # hostname:grblesp"));
    FW version: FluidNC v3.3.0 # FW target:grbl-embedded  # FW HW:No SD  # primary sd:/sd # secondary sd:none # authentication:no # webcommunication: Sync: 81:192.168.0.184 # hostname:fluidnc # axis:3

    */
}


    //login status check
  void WebServer::handleLogin(AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse(200, FST("application/json"), FST("{\"status\":\"Ok\",\"authentication_lvl\":\"admin\"}"));
      response->addHeader(HDR_CACHE_CONTROL, HDR_CC_NO_CACHE);
      request->send(response);
  }

void WebServer::begin() {
  /* Start web server and web socket server */
 // Route for root / web page
 

 /*
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, FST("/index.html"), EMPTY_STRING, false);
  });
  */


  server.on(FST("/"), HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);

  server.on(FAVICON_PATH, HTTP_GET, [](AsyncWebServerRequest *request) {
    if (SPIFFS.exists(FAVICON_PATH)) {
      request->send(SPIFFS, FAVICON_PATH, CT_IMAGE_ICON, false);
      return;
    }
    request->send_P(200, CT_IMAGE_ICON, FAVICON, FAVICON_SIZE);
  });

  //need to be there even no authentication to say to UI no authentication
  server.on(FST("/login"), HTTP_GET, handleLogin);

  //Web Commands
  server.on(FST("/command"), HTTP_ANY, handleWebCommand);
  server.on(FST("/command_silent"), HTTP_ANY, handleWebCommandSilent);

#if ENABLE_SPIFFS
  //SPIFFS
  server.on(FST("/files"), HTTP_ANY, handleSpiffs, handleSpiffsUpload); 
#endif // ENABLE_SPIFFS

  //web update
  server.on(FST("/update"), HTTP_POST, handleUpdate, handleUpdateUpload);

  //Direct SD management
  //server.on(FST("/upload"), HTTP_ANY, handleDirectSDFileList, SDFileDirectUpload);

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  
  server.begin();
  
#if ENABLE_WS_STATE_MONITOR && ENABLE_STATE_REG
  if (RegGroup::mainGroup) {
    RegGroup::mainGroup->addChangeCallback(wsStateChangeCallback, this);
  }
#endif
}

 
/************************************************************************\
|* SPIFFS Request Handler
\************************************************************************/
#if ENABLE_SPIFFS
const char* ARG_PATH PROGMEM = "path";
const char* ARG_ACTION PROGMEM = "action";
const char* ARG_FILENAME PROGMEM = "filename";
const char* ARG_NEW_NAME PROGMEM = "new-name";

void WebServer::handleSpiffs(AsyncWebServerRequest* request) {

    String path = request->hasArg(ARG_PATH) ? request->arg(ARG_PATH) : FST("/");
    path.trim();
    path.replace("//", "/");
    size_t n = path.length();
    while (n>2 && path[n-1] == '/') { n--; }
    path = path.substring(0, n);

    ErrorCode aec = EC_ERROR; 
    char buffer[128];
    PrintBuffer s(buffer, sizeof(buffer));
    if (request->hasArg(ARG_ACTION)) {
      const char* action = request->arg(ARG_ACTION).c_str();
      const char* filename = request->hasArg(ARG_FILENAME) ? request->arg(ARG_FILENAME).c_str() : nullptr;
      const char* newName = request->hasArg(ARG_NEW_NAME) ? request->arg(ARG_NEW_NAME).c_str() : nullptr;
      DEBUG_printf(FST("CMD: %s path:%s\n"), action, path.c_str());
      if (!strcmp(FST("list"), action)) {
        s.print(FST("Ok"));
        aec = EC_OK;
      } else {
        if (path.length() > 1 && path[path.length()-1] != '/') { path += '/'; }
        if (!strcmp(FST("delete_file"), action) && filename) {
          aec = spiffsDeleteFile((path+filename).c_str(), &s);
        }
        else if (!strcmp(FST("delete_dir"), action) && filename) {
          aec = spiffsDeleteDir((path+filename).c_str(), &s);
        }
        else if (!strcmp(FST("create_dir"), action) && filename) {
          aec = spiffsCreateDir((path+filename).c_str(), &s);
        }
        else if (!strcmp(FST("rename_file"), action) && filename && newName) {
          aec = spiffsRename((path+filename).c_str(), (path+newName).c_str(), &s);
        }
        else if (!strcmp(FST("rename_dir"), action) && filename && newName) {
          aec = spiffsRenameDir((path+filename).c_str(), (path+newName).c_str(), &s);
        } else {
          s.print(FST("invalid action: "));
          s.print(action);
          aec = EC_BAD_REQUEST;
        }
        if (path.length() != n) { path = path.substring(0, n); }
      }
    }

    AsyncResponseStream* response = request->beginResponseStream(CT_APP_JSON);
    response->addHeader(HDR_CACHE_CONTROL, HDR_CC_NO_CACHE);
    ErrorCode ec = spiffsFilesJson(path.c_str(), *response, s.length() ? s.c_str() : nullptr); 
    if (!ec) ec = aec;
    if (!ec) { ec = EC_HTTP_OK; }
    response->setCode((int)(ec < 200 ? EC_BAD_REQUEST : ec));
    request->send(response);
}


void WebServer::handleSpiffsUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {

  if (!index) {
    DEBUG_print(FST("SPIFFS Upload URL: ")); DEBUG_println(request->url());
    filename.replace(FST("//"), FST("/"));
    if (filename[0] != '/') { filename = '/' + filename; }
    request->_tempFile = SPIFFS.open(filename + FST(".tmp"), "w");
    DEBUG_print(FST("SPIFFS Upload Start: ")); DEBUG_println(filename);
  }

  if (len) {
    request->_tempFile.write(data, len);
    //DEBUG_printf(FST("SPIFFS Upload Chunk - index:%d len:%d\n"), index, len);
  }

  if (final) {
    DEBUG_printf(FST("SPIFFS Upload complete - size:%d\n"), index + len);
    request->_tempFile.close();

    filename.replace(FST("//"), FST("/"));
    if (filename[0] != '/') { filename = '/' + filename; }
    if (SPIFFS.exists(filename)) {
      DEBUG_print(FST("SPIFFS Upload: deleting existing file: ")); DEBUG_println(filename);
      if (!SPIFFS.remove(filename)) {
        DEBUG_print(FST("SPIFFS Upload: failed to delete existing file: ")); DEBUG_println(filename);
      }
    }
    if (!SPIFFS.rename(filename + FST(".tmp"), filename)) {
      DEBUG_print(FST("SPIFFS Upload: failed to rename file: ")); DEBUG_println(filename);
    }

    AsyncResponseStream* response = request->beginResponseStream(CT_APP_JSON);
    response->addHeader(HDR_CACHE_CONTROL, HDR_CC_NO_CACHE);
    int  nstart = filename.lastIndexOf('/');
    if (nstart != -1) { filename = filename.substring(0, nstart); }
    ErrorCode ec = spiffsFilesJson(filename.c_str(), *response); 
    if (!ec) { ec = EC_HTTP_OK; }
    response->setCode((int)(ec < 200 ? EC_BAD_REQUEST : ec));
    request->send(response);
  }
}

#endif // ENABLE_SPIFFS

void WebServer::handleUpdate(AsyncWebServerRequest *request) {
  request->send(EC_BAD_REQUEST, CT_TEXT_PLAIN, FST("Please upload file"));
}

void WebServer::handleUpdateUpload(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
  static int lastProgress = 0;
  static uint32_t startTs = 0;

  if (!index) {
    DEBUG_printf(FST("Firmware Update: %s\n"), filename.c_str());
    lastProgress = 0;
    startTs = millis();

    // if filename includes spiffs, update the spiffs partition
    int cmd = (filename.indexOf(FST("spiffs")) > -1) ? U_SPIFFS : U_FLASH;
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) {
      Update.printError(Serial);
    }
  }

  if (Update.write(data, len) != len) {
    Update.printError(Serial);
  }

  int progress = (index+len) * 100 / request->contentLength();
  if (progress != lastProgress) {
    char buffer[32];
    StrTool::formatDurationMs(buffer, sizeof(buffer), millis() - startTs);
    DEBUG_printf("Progress: %d%% (%d) / %s\n", progress, index + len, buffer);
    lastProgress = progress;
  }

  if (final) {
    AsyncWebServerResponse *response = request->beginResponse(302, CT_TEXT_PLAIN, FST("Please wait while the device reboots"));
    response->addHeader(FST("Refresh"), FST("20"));  
    response->addHeader(FST("Location"), FST("/"));
    request->send(response);
    if (!Update.end(true)){
      Update.printError(Serial);
    } else {
      char buffer[32];
      StrTool::formatDurationMs(buffer, sizeof(buffer), millis() - startTs);
      DEBUG_printf(FST("Update complete after: %s\n"), buffer);
      Serial.flush();
      delay(500);
      DEBUG_println(FST("Restarting"));
      ESP.restart();
    }
  }
}

bool WebServer::checkCaptivePage(AsyncWebServerRequest* request) {
  if (WiFi.getMode() != WIFI_AP) { return false; }
  String page = PAGE_CAPTIVE;
  String stmp = WiFi.softAPIP().toString();
  //Web address = ip + port
  if (HTTP_PORT != 80) {
      stmp += ':';
      stmp += HTTP_PORT;
  }
  page.replace(KEY_IP, stmp);
  page.replace(KEY_QUERY, request->url());
  request->send(EC_HTTP_OK, CT_TEXT_HTML, page);
  return true;
}

bool WebServer::trySendFile(AsyncWebServerRequest* request, const char* p/*=nullptr*/) {
  char path[128];
  size_t n = 0;
  if (!p) { n = StrTool::urlDecode(path, sizeof(path), request->url().c_str()); }
  else {
    while (*p && n < sizeof(path)-2) { path[n++] = *p++; }
    path[n] = '\0';
  } 
  const char* contentType = getContentType(path);

  // Check if file exists on SPIFFS
  if (SPIFFS.exists(path)) {
      DEBUG_printf(FST("Found SPIFFS file: %s\n"), path);
      AsyncWebServerResponse* response = request->beginResponse(SPIFFS, path, contentType);
      if (n>3 && tolower(path[n-1])=='z' && tolower(path[n-2])=='g' && path[n-3]=='.') {
        response->addHeader(HDR_CONTENT_ENCODING, HDR_CE_GZIP);
      }
      request->send(SPIFFS, path, CT_TEXT_HTML, false);
      return true;
  }

  // Check if gzip file exists on SPIFFS
  if (n < sizeof(path)-4) {
    path[n++] = '.'; path[n++] = 'g'; path[n++] = 'z'; path[n] = '\0';
    if (SPIFFS.exists(path)) {
        DEBUG_printf(FST("Found SPIFFS file: %s\n"), path);
        AsyncWebServerResponse* response = request->beginResponse(SPIFFS, path, contentType);
        response->addHeader(HDR_CONTENT_ENCODING, HDR_CE_GZIP);
        request->send(response);
        return true;
    }
  }
  DEBUG_printf(FST("SPIFFS file: \"%s\" not found\n"), path);
  return false;
}

void WebServer::handleNotFound(AsyncWebServerRequest* request) {
  #if ENABLE_SPIFFS_WEB_CONTENT
  if (trySendFile(request)) { return; }
  #endif

  if (checkCaptivePage(request)) { return; }
  
  #if ENABLE_SPIFFS_WEB_CONTENT
  if (trySendFile(request, FST("/404.htm"))) { return; }
  if (trySendFile(request, FST("/404.html"))) { return; }
  #endif

  // Nothing found use default page
  String page = PAGE_NOT_FOUND;
  String stmp;
  if (WiFi.getMode() == WIFI_STA) {
      stmp = WiFi.localIP().toString();
  } else {
      stmp = WiFi.softAPIP().toString();
  }
  //Web address = ip + port
  if (HTTP_PORT != 80) {
      stmp += ':';
      stmp += HTTP_PORT;
  }
  page.replace(KEY_IP, stmp);
  page.replace(KEY_QUERY, request->url());
  request->send(EC_NOT_FOUND, CT_TEXT_HTML, page);
}

/*
//push error code and message to websocket
void Web_Server::pushError(int code, const char* st, bool web_error, uint16_t timeout) {
    if (_socket_server && st) {
        String s = "ERROR:" + String(code) + ":";
        s += st;
        _socket_server->sendTXT(_id_connection, s);
        if (web_error != 0 && _webserver && _webserver->client().available() > 0) {
            _webserver->send(web_error, "text/xml", st);
        }

        uint32_t start_time = millis();
        while ((millis() - start_time) < timeout) {
            _socket_server->loop();
            delay(10);
        }
    }
}

//abort reception of packages
void Web_Server::cancelUpload() {
    if (_webserver && _webserver->client().available() > 0) {
        HTTPUpload& upload = _webserver->upload();
        upload.status      = UPLOAD_FILE_ABORTED;
        errno              = ECONNABORTED;
        _webserver->client().stop();
        delay(100);
    }
}

//SPIFFS files uploader handle
void Web_Server::SPIFFSFileupload() {
    HTTPUpload& upload = _webserver->upload();
    //this is only for admin and user
    if (is_authenticated() == AuthenticationLevel::LEVEL_GUEST) {
        _upload_status = UploadStatus::FAILED;
        log_info("Upload rejected");
        pushError(ESP_ERROR_AUTHENTICATION, "Upload rejected", 401);
    } else {
        if ((_upload_status != UploadStatus::FAILED) || (upload.status == UPLOAD_FILE_START)) {
            if (upload.status == UPLOAD_FILE_START) {
                String sizeargname = upload.filename + "S";
                size_t filesize    = _webserver->hasArg(sizeargname) ? _webserver->arg(sizeargname).toInt() : 0;
                uploadStart(upload.filename, filesize, "/localfs");
            } else if (upload.status == UPLOAD_FILE_WRITE) {
                uploadWrite(upload.buf, upload.currentSize);
            } else if (upload.status == UPLOAD_FILE_END) {
                String sizeargname = upload.filename + "S";
                size_t filesize    = _webserver->hasArg(sizeargname) ? _webserver->arg(sizeargname).toInt() : 0;
                uploadEnd(filesize, "/localfs");
            } else {  //Upload cancelled
                uploadStop();
                return;
            }
        }
    }
    uploadCheck(upload.filename, "/localfs");
    COMMANDS::wait(0);
}
*/
#endif //ENABLE_WIFI

