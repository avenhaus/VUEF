#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <time.h>
#include <sys/time.h>

#include "VUEF.h"
#if ENABLE_WIFI
#include "Helper.h"
#include "ConfigReg.h"
#include "Command.h"
#include "Network.h"
#include "TcpServer.h"
#include "WebServer.h"

#if __has_include("Secret.h")
  #include "Secret.h"
#endif

#ifndef WIFI_STA_SSID
#define WIFI_STA_SSID "Best WIFI"
#endif

#ifndef WIFI_STA_PASSWORD
#define WIFI_STA_PASSWORD "WIFI Password"
#endif

#ifndef WIFI_AP_SSID
#define WIFI_AP_SSID PROJECT_NAME
#endif

#ifndef WIFI_AP_PASSWORD
// Minimum 8 characters !!
#define WIFI_AP_PASSWORD "12345678"
#endif

// Changing this name also requires change in settings.js: var EP_STA_SSID = "WIFI.SSID"; 
RegGroup configGroupNetwork(FST("WIFI"), nullptr, FST("Network Settings"));

const ConfigEnum::Option configWifiModeOptions[] PROGMEM = {
  { "Off", 0 },
  { "Station", 1 },
  { "Access Point", 2 }
};
const size_t configWifiModeOptionsSize = sizeof(configWifiModeOptions) / sizeof(ConfigEnum::Option);
ConfigEnum configNetworkMode(FST("WIFI Mode"), configWifiModeOptions, configWifiModeOptionsSize, 1, FST("Off/STA/AP"), 0, &configGroupNetwork);

ConfigStr configStaSSID(FST("SSID"), 36, WIFI_STA_SSID, FST("Name of WIFI network"), 0, &configGroupNetwork, 0,0,0,0, RF_WIZARD);
ConfigStr configStaPassword(FST("Password"), 64, WIFI_STA_PASSWORD, FST("Password for WIFI network"), 0, &configGroupNetwork, 0,0,0,0, RF_PASSWORD | RF_WIZARD);
ConfigStr configHostname(FST("Hostname"), 32, HOSTNAME, FST("Name of this device on the network"), 0, &configGroupNetwork);

ConfigIpAddr configIpAddr(FST("IP"), 0, FST("Fixed IP address of this device"), 0, &configGroupNetwork);
ConfigIpAddr configGateway(FST("Gateway"), 0, FST("Gateway IP address"), 0, &configGroupNetwork);
ConfigIpAddr configSubnet(FST("Subnet"), 0, FST("Subnet mask"), 0, &configGroupNetwork);
ConfigIpAddr configDNS(FST("DNS"), 0, FST("Domain Name Server"), 0, &configGroupNetwork);
const uint8_t AP_IP_ADR[4] PROGMEM = {192, 168, 0, 4};
ConfigIpAddr configApIpAddr(FST("AP IP"), AP_IP_ADR, FST("IP address of this device in Access Point mode"), 0, &configGroupNetwork);
ConfigStr configApSSID(FST("AP SSID"), 36, WIFI_AP_SSID, FST("Name of WIFI network in Access Point mode"), 0, &configGroupNetwork);
ConfigStr configApPassword(FST("AP Password"), 64, WIFI_AP_PASSWORD, FST("Password for WIFI network in Access Point mode"), 0, &configGroupNetwork);
ConfigUInt8 configApChannel(FST("AP Channel"), 1, FST("WIFI channel in Access Point mode"), 0, &configGroupNetwork);

#if ENABLE_NTP
ConfigStr configTimeZone(FST("Time Zone"), 48, TIME_ZONE, FST("Time zone used for NTP"), 0, &configGroupNetwork, 0,0,0,0, RF_WIZARD);
ConfigStr configTimeFormat(FST("Time Format"), 16, TIME_FORMAT, FST("Format string used to print time"), 0, &configGroupNetwork);
ConfigStr configDateFormat(FST("Date Format"), 16, DATE_FORMAT, FST("Format string used to print date"), 0, &configGroupNetwork);
#endif

// StateStr stateWifiConnection(FST("Connection"), FST("Not connected"), FST("WIFI connection state"), 0, &configGroupNetwork);

#if ENABLE_CLI
Command cmdListNetorksJson(FST("networks"), 
[] (const char* args, Print* stream) {
  if (!stream) { return EC_OK; }
  return listWifiNetworksJson(*stream);
},
FST("List WIFI networks as JSON"),
nullptr, nullptr, CT_APP_JSON
);
#endif // ENABLE_CLI


int setenv(const char *, const char *, int);
void tzset();

void web_server_init();

TcpServer* tcpServer;
WebServer* webServer;

bool gotNtp = false;
char fullHostname[64] = "";

void otaInit(const char* fullHostname);
void wsRun();


/*--------------------------------------------------------------*\
 * Wait for WIFI Station to connect to Access Point
\*--------------------------------------------------------------*/     
bool wifiWaitForStaConnect() {
    size_t count = 0;
    for (size_t i = 0; i < 50; ++i) {
        switch (WiFi.status()) {
            case WL_NO_SSID_AVAIL:
                DEBUG_println(FST("\nNo SSID"));
                isConfigOk = false;
                return false;
            case WL_CONNECT_FAILED:
                DEBUG_println(FST("\nConnection failed"));
                isConfigOk = false;
                return false;
            case WL_CONNECTED:
                DEBUG_print(FST("\nConnected - Local IP: "));
                DEBUG_println(WiFi.localIP());
                return true;
            default:
                if (!count) { DEBUG_print(FST("Connecting WIFI "));  }
                else if ((count & 3) == 0) { DEBUG_print('.'); }
                count++;
                break;
        }
        delay(100);
    }
    DEBUG_println(FST("\nTimed out connecting WIFI"));
    isConfigOk = false;
    return false;
}

/*--------------------------------------------------------------*\
 * Start WIFI client mode (Station)
\*--------------------------------------------------------------*/     
bool wifiStartSTA() {
    wifiServicesStop();
    //Sanity check
    if ((WiFi.getMode() == WIFI_STA) || (WiFi.getMode() == WIFI_AP_STA)) {
        WiFi.disconnect();
    }
    if ((WiFi.getMode() == WIFI_AP) || (WiFi.getMode() == WIFI_AP_STA)) {
        WiFi.softAPdisconnect();
    }
    WiFi.enableAP(false);

    if (!configStaSSID.get() || !configStaSSID.get()[0]) {
        DEBUG_println(FST("STA SSID is not set"));
        return false;
    }

    WiFi.mode(WIFI_STA);

    // Configure the hostname
    uint8_t mac[6];
    WiFi.macAddress(mac);
    snprintf(fullHostname, sizeof(fullHostname)-1, "%s-%02X%02X", HOSTNAME, mac[4], mac[5]);
    WiFi.setHostname(fullHostname);

    if (configIpAddr.isSet()) {
        IPAddress ip(configIpAddr.get());
        IPAddress mask(configSubnet.get());
        IPAddress gateway(configGateway.get());
        WiFi.config(ip, gateway, mask);
    }
    const char* pw = configStaPassword.get();
    if (pw && !pw[0]) { pw = nullptr; }
    if (WiFi.begin(configStaSSID.get(), pw)) {
        DEBUG_printf(FST("\nConnecting STA WIFI: \"%s\" with hostname: %s\n"), configStaSSID.get(), fullHostname);
        return wifiWaitForStaConnect();
    }
    DEBUG_println(FST("Starting WIFI STA failed"));
    return false;
}

/*--------------------------------------------------------------*\
 * Setup and start Access Point
\*--------------------------------------------------------------*/     
bool wifiStartAP() {
    //Sanity check
    if ((WiFi.getMode() == WIFI_STA) || (WiFi.getMode() == WIFI_AP_STA)) {
        WiFi.disconnect();
    }
    if ((WiFi.getMode() == WIFI_AP) || (WiFi.getMode() == WIFI_AP_STA)) {
        WiFi.softAPdisconnect();
    }

    WiFi.enableSTA(false);
    WiFi.mode(WIFI_AP);

    IPAddress ip(configApIpAddr.get());
    IPAddress mask({255,255,255,0});
    char ips[20];
    configApIpAddr.toStr(ips, sizeof(ips));
    DEBUG_printf(FST("AP SSID: \"%s\"  IP %s  password: %s  channel: %d\n"), configApSSID.get(), ips, configApPassword.get(), configApChannel.get() );

    //Set static IP
    WiFi.softAPConfig(ip, ip, mask);

    //Start AP
    const char* pw = configApPassword.get();
    if (pw && !pw[0]) { pw = nullptr; }
    if (WiFi.softAP(configApSSID.get(), pw, configApChannel.get())) {
        DEBUG_println(FST("AP started"));
        return true;
    }
    DEBUG_println(FST("AP did not start"));
    return false;
}

/*--------------------------------------------------------------*\
 * Stop WiFi
\*--------------------------------------------------------------*/     
void wifiReset() {
    WiFi.persistent(false);
    WiFi.disconnect(true);
    WiFi.enableSTA(false);
    WiFi.enableAP(false);
    WiFi.mode(WIFI_OFF);
}

void wifiStop() {
    if (WiFi.getMode() != WIFI_MODE_NULL) {
        if ((WiFi.getMode() == WIFI_STA) || (WiFi.getMode() == WIFI_AP_STA)) {
            WiFi.disconnect(true);
        }
        if ((WiFi.getMode() == WIFI_AP) || (WiFi.getMode() == WIFI_AP_STA)) {
            WiFi.softAPdisconnect(true);
        }
        wifiServicesStop();
        WiFi.enableSTA(false);
        WiFi.enableAP(false);
        WiFi.mode(WIFI_OFF);
    }
    DEBUG_println(FST("WiFi Off"));
}

/*--------------------------------------------------------------*\
 * Start WiFi
\*--------------------------------------------------------------*/     
bool wifiStart() {
    wifiServicesStop();
    uint8_t mode = configNetworkMode.get();
    bool isEnabled = false;
    if (mode == 0) {
        DEBUG_println(FST("WiFi is disabled"));
        return false;
    } else if (mode == 1) {
      isEnabled = wifiStartSTA();
    } 
    if (mode == 2 || !isEnabled) {
      isEnabled = wifiStartAP();
    }
    if (!isEnabled) { 
      return false;
    }
    return wifiServicesStart();
}

/*--------------------------------------------------------------*\
 * Start WiFi Services
\*--------------------------------------------------------------*/     
bool wifiServicesStart() {
    DEBUG_print(FST("WiFi connected. IP address: http://"));
    DEBUG_println(getLocalIp());

#if ENABLE_MDNS
    if (!MDNS.begin(fullHostname)) {
      DEBUG_println("Cannot start mDNS");
    } else {
      MDNS.addService("http", "tcp", HTTP_PORT);
      DEBUG_printf(FST("mDNS: http://%s.local\n"), fullHostname);
    }
#endif

    
#if ENABLE_OTA
    otaInit(fullHostname);
#endif

#if ENABLE_TELNET
    tcpServer = new TcpServer();
    tcpServer->begin();
    DEBUG_println(FST("Started Telnet"));
#endif

#if ENABLE_WEB_SERVER
    webServer = new WebServer();
    webServer->begin();
    DEBUG_println(FST("Started Web Server"));
#endif

#if ENABLE_NTP
  configTime(0, 0, NTP_SERVER_1, NTP_SERVER_2);
  setenv("TZ", configTimeZone.get(), 1);
  tzset();
#endif

  WiFi.scanNetworks(true); // Start Async scan so it's available later

#ifdef USE_NETWORK_TASK
    xTaskCreate(
        networkTask_,   // Task function
        "net",          // String with name of task
        2048,           // Stack size in bytes
        NULL,           // Parameter passed as input of the task
        1,              // Priority of the task.
        NULL);          // Task handle.
#endif
  return true;
}

void wifiServicesStop() {

}



void networkInit() {
  wifiStart();
}

void networkRun() {
  #if ENABLE_OTA
  ArduinoOTA.handle();
  #endif

  #if ENABLE_TELNET
  tcpServer->run();
  #endif

  #if ENABLE_WEB_SERVER
  webServer->run();
  #endif

}

IPAddress getLocalIp() { return WiFi.getMode() == WIFI_STA ? WiFi.localIP() : WiFi.softAPIP(); }
String getWifiMac() { return WiFi.macAddress(); }
int getRSSI() { return WiFi.RSSI(); }


#if ENABLE_NTP
void getNtpTime() {
  uint32_t start = millis();
  gotNtp = false;
  while(getEpochTime() < 50000) {
    if ((millis()-start) > 6000) {
      DEBUG_println(F("\nFailed to get NTP time."));
      break;
    }
    delay(1);
  }
  gotNtp = true;
  char iso[32];
  getIsoTime(iso);
  DEBUG_print(F("NTP: "));
  DEBUG_print(gotNtp);
  DEBUG_print(F(" Time: "));
  DEBUG_print(iso);
  DEBUG_print(F(" Epoch: "));
  DEBUG_print(getEpochTime());
  DEBUG_print(F(" | NTP duration: "));
  DEBUG_println((millis() - start) * 0.001);  
  char bu[64];
  getTimeStr(bu, FST("%d.%m.%y %H:%M"));
  DEBUG_println(bu);
}
#endif

size_t getWifiId(char* buffer, size_t bSize) {
  size_t n = 0;
  const char* o = WiFi.macAddress().c_str();
  while (*o && n < bSize-1) {
    if (*o == ':') { o++; continue; }
    buffer[n++] = *o++;
  }
  buffer[n] = 0;
  return n;
}

time_t getEpochTime() {
  time_t now = 0;
  time(&now);
  return now;
}

uint32_t getMillisDelay(uint32_t freq) {
  timeval tv;
  timezone tz;
  gettimeofday(&tv, &tz);
  struct tm * tm = localtime(&tv.tv_sec);
  uint32_t millis = (tm->tm_hour * 60*60*1000) + (tm->tm_min * 60*1000) + (tm->tm_sec * 1000) + (tv.tv_usec/1000); 
  return freq - (millis % freq) + 1;
}

size_t getIsoTime(char* buffer) {
  // 2018-05-27T21:33:19Z
  time_t now = 0;
  time(&now);
  return strftime(buffer, 32, FST("%FT%TZ"), gmtime(&now));
}

size_t getTimeStr(char* buffer, const char* fmt) {
  time_t now = 0;
  time(&now);
  return strftime(buffer, 32, fmt, localtime(&now));
}


int32_t getWifiSignalStrength(int32_t RSSI) {
    if (RSSI <= -100) { return 0; }
    if (RSSI >= -50) { return 100; }
    return 2 * (RSSI + 100);
}

void printMac(Print& s, uint8_t *mac) {
  s.printf(FST("%02X:%02X:%02X:%02X:%02X:%02X"), mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

/*----------------------------------------------------------------------*\
|* List WIFI Networks as JSON
\*----------------------------------------------------------------------*/
 ErrorCode listWifiNetworksJson(Print& out, bool filterEmpty/*=true*/) {
  bool gotScan = false;
  uint32_t timeout = millis() + 5000;
  size_t count = 0;
  while (!gotScan) {
    if (millis() > timeout) {
      out.print(FST("[]"));
      return EC_ERROR;
    }
    // An initial async scanNetworks was issued at startup, so there
    // is a good chance that scan information is already available.
    int n = WiFi.scanComplete();
    switch (n) {
        case -2:                      // Scan not triggered
            WiFi.scanNetworks(true);  // Begin async scan
            delay(25);
            break;
    
        case -1:  // Scan in progress
            delay(25);
            break;
    
        default:
            out.write('[');
            for (int i = 0; i < n; ++i) {
                if (!filterEmpty || WiFi.SSID(i).length()) {
                  if (count) { out.write(','); }
                  out.print(FST("{\"SSID\":\"")); out.print(WiFi.SSID(i));
                  out.print(FST("\",\"SIG\":")); out.print(getWifiSignalStrength(WiFi.RSSI(i)));
                  out.print(FST(",\"ENC\":")); out.print(WiFi.encryptionType(i));
                  out.write('}');
                  count++;
                }
            }
            out.write(']');
            gotScan = true;
            WiFi.scanDelete();
            n = WiFi.scanComplete();
            if (n == -2) {
                WiFi.scanNetworks(true);
            }
            break;
    }
  }
  return EC_OK;
}


/*----------------------------------------------------------------------*\
|* Wifi Info
\*----------------------------------------------------------------------*/

#include <esp_wifi.h>
#include <esp_ota_ops.h>
void wifiInfo(Print& s) {
  char buffer[64];  
  s.print(FST("WIFI Sleep mode: ")); s.println(WiFi.getSleep() ? FST("Modem") : FST("None"));
    int mode = WiFi.getMode();
    if (mode != WIFI_MODE_NULL) {
        //Is OTA available ?
        size_t flashsize = 0;
        if (esp_ota_get_running_partition()) {
            const esp_partition_t* partition = esp_ota_get_next_update_partition(NULL);
            if (partition) {
                flashsize = partition->size;
            }
        }
        StrTool::formatBytes(buffer, sizeof(buffer), flashsize);
        s.print(FST("Available Size for OTA update: ")); s.println(buffer);
        s.printf(FST("HTTP port: %d\r\n"), HTTP_PORT); 
        s.printf(FST("Telnet port: %d\r\n"), TELNET_PORT); 
        s.print(FST("Hostname: ")); s.println(configHostname.get());
        s.print(FST("Full Hostname: ")); s.println(fullHostname);
    }

    s.print(FST("Current WiFi Mode: "));
    switch (mode) {
        case WIFI_STA:
            s.printf(FST("STA(%s)\r\n"), WiFi.macAddress().c_str());

            s.print(FST("Connected to: "));
            if (WiFi.isConnected()) {  //in theory no need but ...
                s.println(WiFi.SSID());
                s.print(FST("Signal: ")); s.println(getWifiSignalStrength(WiFi.RSSI()));

                uint8_t PhyMode;
                esp_wifi_get_protocol(WIFI_IF_STA, &PhyMode);
                const char* modeName;
                switch (PhyMode) {
                    case WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N:
                        modeName = FST("11n");
                        break;
                    case WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G:
                        modeName = FST("11g");
                        break;
                    case WIFI_PROTOCOL_11B:
                        modeName = FST("11b");
                        break;
                    default:
                        modeName = FST("???");
                }
                s.print(FST("Phy Mode: ")); s.println(modeName);
                s.print(FST("Channel: ")); s.println(WiFi.channel());

                tcpip_adapter_dhcp_status_t dhcp_status;
                tcpip_adapter_dhcpc_get_status(TCPIP_ADAPTER_IF_STA, &dhcp_status);
                s.print(FST("IP Mode: ")); s.println(dhcp_status == TCPIP_ADAPTER_DHCP_STARTED ? FST("DHCP") : FST("Static"));
                s.print(FST("IP: ")); s.println(getLocalIp());
                s.print(FST("Gateway: ")); s.println(WiFi.gatewayIP());
                s.print(FST("Mask: ")); s.println(WiFi.subnetMask());
                s.print(FST("DNS: ")); s.println(WiFi.dnsIP());

            }  //this is web command so connection => no command
            s.printf(FST("Disabled Mode: AP(%s)\r\n"), WiFi.softAPmacAddress().c_str());
            break;
        case WIFI_AP:
            s.printf(FST("AP(%s)\r\n"), WiFi.softAPmacAddress().c_str());

            wifi_config_t conf;
            esp_wifi_get_config(WIFI_IF_AP, &conf);
            s.print(FST("SSID: ")); s.println((const char*)conf.ap.ssid);
            s.print(FST("Visible: ")); s.println(conf.ap.ssid_hidden == 0 ? FST("Yes") : FST("No"));

            const char* mode;
            switch (conf.ap.authmode) {
                case WIFI_AUTH_OPEN:
                    mode = FST("None");
                    break;
                case WIFI_AUTH_WEP:
                    mode = FST("WEP");
                    break;
                case WIFI_AUTH_WPA_PSK:
                    mode = FST("WPA");
                    break;
                case WIFI_AUTH_WPA2_PSK:
                    mode = FST("WPA2");
                    break;
                default:
                    mode = FST("WPA/WPA2");
            }

            s.print(FST("Authentication: ")); s.println(mode);
            s.print(FST("Max Connections: ")); s.println(conf.ap.max_connection);

            tcpip_adapter_dhcp_status_t dhcp_status;
            tcpip_adapter_dhcps_get_status(TCPIP_ADAPTER_IF_AP, &dhcp_status);
            s.print(FST("DHCP Server: ")); s.println(dhcp_status == TCPIP_ADAPTER_DHCP_STARTED ? FST("Started") : FST("Stopped"));

            s.print(FST("IP: ")); s.println(WiFi.softAPIP().toString());

            tcpip_adapter_ip_info_t ip_AP;
            tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP, &ip_AP);
            s.print(FST("Gateway: ")); s.println(IPAddress(ip_AP.gw.addr));
            s.print(FST("Mask: ")); s.println(IPAddress(ip_AP.netmask.addr));

            wifi_sta_list_t          station;
            tcpip_adapter_sta_list_t tcpip_sta_list;
            esp_wifi_ap_get_sta_list(&station);
            tcpip_adapter_get_sta_list(&station, &tcpip_sta_list);
            s.print(FST("Connected channels: ")); s.println(station.num);

            for (int i = 0; i < station.num; i++) {
                printMac(s, tcpip_sta_list.sta[i].mac);
                s.write(' ');
                s.println(IPAddress(tcpip_sta_list.sta[i].ip.addr).toString());
            }
            s.print(FST("Disabled Mode: "));
            s.printf(FST("STA(%s)\r\n"), WiFi.macAddress().c_str());
            break;
        case WIFI_AP_STA:  //we should not be in this state but just in case ....
            s.println(FST("Mixed"));

            s.printf(FST("STA(%s)\r\n"), WiFi.macAddress().c_str());
            s.printf(FST("AP(%s)\r\n"), WiFi.softAPmacAddress().c_str());
            break;
        default:  //we should not be there if no wifi ....
            s.println(FST("Off"));
            break;
    }
    s.println();
}
#endif // ENABLE_WIFI
