#ifndef WIFICONFIGMANAGER_H
#define WIFICONFIGMANAGER_H
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include "ESPAsyncWebServer.h"
#include "AsyncTCP.h"
#include <PubSubClient.h>
#include <Update.h>

// --- Strukturen und Enums ---
enum FormType { STRING, FLOAT, BOOL, LONG };
enum LineType { CONFIGBLOCK, TITLE, SUBTITLE, SEPARATOR, BLANK, PARAMETER };

struct ConfigStruc {
  char title[64];
  char ssid[64];
  char ssidpasswd[64];
  char mdns[64];
  char mqttIp[16];
  int  mqttPort;
  char mqttUser[64];
  char mqttPasswd[64];
  bool configured;
};

struct ExtraStruc {
  char     keyName[16]; // max. 15 + \0
  FormType formType;
  char     TEXTvalue[64];
  float    FLOATvalue;
  bool     BOOLvalue;
  long     LONGvalue;
  bool     optional;
  bool     inputParam;
};

struct WebStruc {
  LineType lineType;
  char     label[64];
  char     relatedKey[16];
};

class WifiConfigManager {
public:
  WifiConfigManager(ConfigStruc* config,
                    ExtraStruc* extraParams,
                    const WebStruc* webForm,
                    int webFormCount,
                    int anzExtraparams,
                    const char* firmwareVersion);
  ~WifiConfigManager();

  // Lebenszyklus
  void begin(const String& apPrefix = "myESP-Setup");
  void handleLoop();

  // Getter
  String getSSID();
  String getPassword();
  String getMqttServer();
  int    getMqttPort();
  String getMqttUser();
  String getMqttPassword();
  String getMdnsName();
  bool   isWifiConnected();
  bool   isMqttConnected();
  int    getRSSI();

  // Extra-Parameter
  String getExtraParam(const char* keyName);
  int    getExtraParamInt(const char* keyName);
  float  getExtraParamFloat(const char* keyName);
  bool   getExtraParamBool(const char* keyName);

  // Persistenz
  void loadConfig();
  void saveConfig();

  // MQTT-Hilfen (NEU)
  bool ensureMqttConnected();
  bool publish(const char* topic, const String& payload, bool retain=false, int qos=0);

private:
  // Netzwerk & Persistenz
  AsyncWebServer _server;
  Preferences    _prefsNetwork;
  Preferences    _prefsOperation;
  WiFiClient     _wifiClient;
  PubSubClient   _mqttClient;

  // Basiskonfig
  String _apNamePrefix;
  int    _anzExtraparams;
  const char* _firmwareVersion;

  // Referenzen
  ConfigStruc*    _config;
  ExtraStruc*     _extraParams;
  const WebStruc* _webForm;
  int             _webFormCount;

  // intern
  void _startAP();
  void _connectToWiFi();
  void _setupMDNS();
  void _reconnectMQTT();
  String _getHtmlForm();
  int  _findExtraParamIndex(const char* keyName);

  bool  _validateForm(AsyncWebServerRequest* request);
  String _getValidationErrorHtml(const String& errorList);
};

#endif
