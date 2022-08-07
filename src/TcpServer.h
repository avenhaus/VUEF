/*

 */

#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <cstddef>
#include "WiFiServer.h"
#include "CLI.h"

#ifndef MAX_TCP_CONNECTIONS
#define MAX_TCP_CONNECTIONS 3
#endif

#ifndef TELNET_PORT
#define TELNET_PORT 23
#endif

class TcpConnection : public Stream {
public:
  typedef void (*GotDataCB)(char* line, size_t len, void* data);
  enum AcceptState { Accepted, Busy, NoConnection };
  TcpConnection() :  gotDataCB(nullptr), buffer_index(0), cmdli(this) { buffer[0] = 0; }
  AcceptState accept(WiFiServer& server);
  AcceptState connect(const char* host, uint16_t port=23, int32_t timeout=0, GotDataCB cb=nullptr, void* cbData=nullptr);
  inline bool isConnected() { return connection.connected(); }
  void run();


protected:

  // Stream implementation
  virtual int read();
  virtual int available();
  virtual int peek();
  virtual size_t write(uint8_t val);
  using Print::write; // pull in write(str) and write(buf, size) from Print
  virtual void flush();

  bool readLine();
  char readHex();

  WiFiClient connection;
  GotDataCB gotDataCB;
  void* cbData;
  char buffer[COMMAND_BUFFER_SIZE];
  size_t buffer_index;
  size_t len;
  CommandLineInterpreter cmdli;
};


class TcpServer {
public:
  TcpServer(uint16_t port=TELNET_PORT)  : server(port) { }
  void begin() { server.begin(); }
  void run();

protected:
  WiFiServer server;
  TcpConnection tcp_connections[MAX_TCP_CONNECTIONS];
};

#endif // TCP_SERVER_H
