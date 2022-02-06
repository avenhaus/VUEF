#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <cstddef>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "VUEF.h"
#include "StateReg.h"


#ifndef MAX_TCP_CONNECTIONS
#define MAX_TCP_CONNECTIONS 3
#endif

#ifndef WS_BUFFER_SIZE
#define WS_BUFFER_SIZE 512
#endif

#ifndef HTTP_PORT
#define HTTP_PORT 80
#endif

extern const char CT_TEXT_PLAIN[] PROGMEM;
extern const char CT_TEXT_HTML[] PROGMEM;
extern const char CT_TEXT_CSS[] PROGMEM;
extern const char CT_TEXT_XML[] PROGMEM;
extern const char CT_IMAGE_PNG[] PROGMEM;
extern const char CT_IMAGE_GIF[] PROGMEM;
extern const char CT_IMAGE_JPG[] PROGMEM;
extern const char CT_IMAGE_ICON[] PROGMEM;
extern const char CT_APP_YAML[] PROGMEM;
extern const char CT_APP_JSON[] PROGMEM;
extern const char CT_APP_JS[] PROGMEM;
extern const char CT_APP_PDF[] PROGMEM;
extern const char CT_APP_ZIP[] PROGMEM;
extern const char CT_APP_GZ[] PROGMEM;
extern const char CT_APP_OCT[] PROGMEM;

typedef struct ExtToCt {
  const char ext[6];
  const char* ct;
} ExtToCt;

const char* getContentType(const char* path);

class WebServer {
public:
  enum WsState { NoConnection, Connected, Ready };
  WebServer(uint16_t port=HTTP_PORT, const char* wsPath = FST("/ws"))  : server(port), ws(wsPath), wsState(NoConnection) { }
  void begin();
  void run();
  inline bool isConnected() { return wsState != NoConnection; }
  inline bool isReady() { return wsState == Ready; }

protected:
  static bool checkCaptivePage(AsyncWebServerRequest* request);
  static bool trySendFile(AsyncWebServerRequest* request, const char* p=nullptr);
  static void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);
  static void handleRoot(AsyncWebServerRequest* request);
  static void handleLogin(AsyncWebServerRequest* request);
  static void handleNotFound(AsyncWebServerRequest* request); 
  static void handleWebCommand_(AsyncWebServerRequest* request, bool);
  static void handleWebCommand(AsyncWebServerRequest* request) { handleWebCommand_(request, false); }
  static void handleWebCommandSilent(AsyncWebServerRequest* request) { handleWebCommand_(request, true); }
  static void handleSpiffs(AsyncWebServerRequest* request);
  static void handleSpiffsUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
  static void handleUpdate(AsyncWebServerRequest* request);
  static void handleUpdateUpload(AsyncWebServerRequest* request, const String& filename, size_t index, uint8_t *data, size_t len, bool final);
  static void cancelUpload(AsyncWebServerRequest* request);
  static void handleDirectSDFileList(AsyncWebServerRequest* request);
  static void SDFileDirectUpload(AsyncWebServerRequest* request);

  static void pushError(int code, const char* st, bool web_error = 500, uint16_t timeout = 1000);

#if ENABLE_WS_STATE_MONITOR && ENABLE_STATE_REG
  static void wsStateChangeCallback(RegGroup& group, void* data);
  void wsStateChangeCallback_(RegGroup& group);
#endif


  AsyncWebServer server;
  AsyncWebSocket ws;
  WsState wsState;

  static const char PAGE_NOT_FOUND[] PROGMEM;
  static const char PAGE_CAPTIVE[] PROGMEM;
};

#endif // WEB_SERVER_H
