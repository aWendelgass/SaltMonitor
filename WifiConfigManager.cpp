#include "WifiConfigManager.h"
#include <PubSubClient.h>

// Preferences Namespaces
static const char* PREFS_NAMESPACE_NETWORK   = "network";
static const char* PREFS_NAMESPACE_OPERATION = "operation";

WifiConfigManager::WifiConfigManager(ConfigStruc* config,
                                     ExtraStruc* extraParams,
                                     const WebStruc* webForm,
                                     int webFormCount,
                                     int anzExtraparams)
: _server(80), _mqttClient(_wifiClient), _config(config), _extraParams(extraParams),
  _webForm(webForm), _webFormCount(webFormCount), _anzExtraparams(anzExtraparams) {}

WifiConfigManager::~WifiConfigManager() {}

void WifiConfigManager::begin(const String& apPrefix) {
  _apNamePrefix = apPrefix;
  loadConfig();

  Serial.print("DEBUG: Status nach loadConfig(): _config->configured = ");
  Serial.println(_config->configured ? "true" : "false");

  if (_config->configured) {
    Serial.println("Gespeicherte Konfiguration gefunden. Versuche zu verbinden...");
    _connectToWiFi();
  } else {
    Serial.println("Keine gespeicherte Konfiguration gefunden. Starte im AP-Modus.");
    _startAP();
  }
}

void WifiConfigManager::handleLoop() {
  if (isWifiConnected() && !isMqttConnected()) { _reconnectMQTT(); }
  _mqttClient.loop();
}

bool   WifiConfigManager::isWifiConnected() { return (WiFi.status() == WL_CONNECTED); }
bool   WifiConfigManager::isMqttConnected() { return _mqttClient.connected(); }
String WifiConfigManager::getSSID()         { return _config->ssid; }
String WifiConfigManager::getPassword()     { return _config->ssidpasswd; }
String WifiConfigManager::getMqttServer()   { return _config->mqttIp; }
int    WifiConfigManager::getMqttPort()     { return _config->mqttPort; }
String WifiConfigManager::getMqttUser()     { return _config->mqttUser; }
String WifiConfigManager::getMqttPassword() { return _config->mqttPasswd; }
String WifiConfigManager::getMdnsName()     { return _config->mdns; }
int    WifiConfigManager::getRSSI()         { return WiFi.RSSI(); }

String WifiConfigManager::getExtraParam(const char* keyName) {
  int index = _findExtraParamIndex(keyName);
  if (index != -1) return _extraParams[index].TEXTvalue;
  return "";
}
int   WifiConfigManager::getExtraParamInt(const char* keyName) {
  int index = _findExtraParamIndex(keyName);
  if (index != -1) return _extraParams[index].LONGvalue;
  return 0;
}
float WifiConfigManager::getExtraParamFloat(const char* keyName) {
  int index = _findExtraParamIndex(keyName);
  if (index != -1) return _extraParams[index].FLOATvalue;
  return 0.0f;
}
bool  WifiConfigManager::getExtraParamBool(const char* keyName) {
  int index = _findExtraParamIndex(keyName);
  if (index != -1) return _extraParams[index].BOOLvalue;
  return false;
}

void WifiConfigManager::loadConfig() {
  _prefsNetwork.begin(PREFS_NAMESPACE_NETWORK, true);
  _prefsOperation.begin(PREFS_NAMESPACE_OPERATION, true);

  _config->configured = _prefsNetwork.getBool("configured", false);
  if (!_config->configured) {
    _prefsNetwork.end();
    _prefsOperation.end();
    Serial.println("******************************** KEINE Konfiguration in Preferences gefunden.");
    return;
  }

  Serial.println("++++++++++++++++++++++++ ERFOLGREICH Konfiguration aus Preferences gelesen.");
  String s;
  s = _prefsNetwork.getString("ssid", "");           strncpy(_config->ssid,      s.c_str(), sizeof(_config->ssid));
  s = _prefsNetwork.getString("ssidpasswd", "");     strncpy(_config->ssidpasswd,s.c_str(), sizeof(_config->ssidpasswd));
  s = _prefsNetwork.getString("mdns", "myESP");      strncpy(_config->mdns,      s.c_str(), sizeof(_config->mdns));
  s = _prefsNetwork.getString("mqttIp", "");         strncpy(_config->mqttIp,    s.c_str(), sizeof(_config->mqttIp));
  s = _prefsNetwork.getString("mqttUser", "");       strncpy(_config->mqttUser,  s.c_str(), sizeof(_config->mqttUser));
  s = _prefsNetwork.getString("mqttPasswd", "");     strncpy(_config->mqttPasswd,s.c_str(), sizeof(_config->mqttPasswd));
  _config->mqttPort = _prefsNetwork.getInt("mqttPort", 1883);

  for (int i = 0; i < _anzExtraparams; i++) {
    const char* key = _extraParams[i].keyName;
    switch (_extraParams[i].formType) {
      case STRING: {
        s = _prefsOperation.getString(key, _extraParams[i].TEXTvalue);
        strncpy(_extraParams[i].TEXTvalue, s.c_str(), sizeof(_extraParams[i].TEXTvalue));
        break; }
      case FLOAT:
        _extraParams[i].FLOATvalue = _prefsOperation.getFloat(key, _extraParams[i].FLOATvalue);
        break;
      case BOOL:
        _extraParams[i].BOOLvalue  = _prefsOperation.getBool (key, _extraParams[i].BOOLvalue);
        break;
      case LONG:
        _extraParams[i].LONGvalue  = _prefsOperation.getLong (key, _extraParams[i].LONGvalue);
        break;
    }
  }
  _prefsOperation.end();
  _prefsNetwork.end();
}

void WifiConfigManager::saveConfig() {
  _prefsNetwork.begin  (PREFS_NAMESPACE_NETWORK,   false);
  _prefsOperation.begin(PREFS_NAMESPACE_OPERATION, false);

  _prefsNetwork.putString("ssid",       _config->ssid);
  _prefsNetwork.putString("ssidpasswd", _config->ssidpasswd);
  _prefsNetwork.putString("mdns",       _config->mdns);
  _prefsNetwork.putString("mqttIp",     _config->mqttIp);
  _prefsNetwork.putInt   ("mqttPort",   _config->mqttPort);
  _prefsNetwork.putString("mqttUser",   _config->mqttUser);
  _prefsNetwork.putString("mqttPasswd", _config->mqttPasswd);
  _prefsNetwork.putBool  ("configured", true);

  for (int i = 0; i < _anzExtraparams; i++) {
    const char* key = _extraParams[i].keyName;
    switch (_extraParams[i].formType) {
      case STRING: _prefsOperation.putString(key, _extraParams[i].TEXTvalue); break;
      case FLOAT:  _prefsOperation.putFloat (key, _extraParams[i].FLOATvalue); break;
      case BOOL:   _prefsOperation.putBool  (key, _extraParams[i].BOOLvalue);  break;
      case LONG:   _prefsOperation.putLong  (key, _extraParams[i].LONGvalue);  break; // FIX: putLong statt putInt
    }
  }

  _prefsOperation.end();
  _prefsNetwork.end();
  Serial.println("Konfiguration in Preferences gespeichert.");
}

void WifiConfigManager::_startAP() {
  uint64_t chipId = ESP.getEfuseMac();
  char macSuffix[5];
  sprintf(macSuffix, "%04X", (uint16_t)(chipId >> 32));
  String ap_ssid = _apNamePrefix + "-" + macSuffix;

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid.c_str());

  _server.on("/", HTTP_GET,
    [this](AsyncWebServerRequest* request){
      request->send(200, "text/html", _getHtmlForm());
    }
  );

  _server.on("/save", HTTP_POST,
    [this](AsyncWebServerRequest* request){
      if (_validateForm(request)) {
        saveConfig();
        request->send(200, "text/html; charset=utf-8",
          "<h1>Konfiguration erfolgreich! Gerät wird neu gestartet.</h1>"
          "<script>setTimeout(function(){window.location.href='/'},3000);</script>");
        delay(1500);
        ESP.restart();
      }
    }
  );
  _server.begin();
  Serial.println("AP-Modus gestartet.");
}

void WifiConfigManager::_connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(_config->ssid, _config->ssidpasswd);
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 40) { delay(500); Serial.print("."); retries++; }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nVerbindung erfolgreich!");
    _setupMDNS();
    _reconnectMQTT();

    _server.on("/", HTTP_GET,
      [this](AsyncWebServerRequest* request){
        request->send(200, "text/html", _getHtmlForm());
      }
    );

    _server.on("/save", HTTP_POST,
      [this](AsyncWebServerRequest* request){
        if (_validateForm(request)) {
          saveConfig();
          request->send(200, "text/html; charset=utf-8",
            "<h1>Konfiguration erfolgreich! Gerät wird neu gestartet.</h1>"
            "<script>setTimeout(function(){window.location.href='/'},3000);</script>");
          delay(1500);
          ESP.restart();
        }
      }
    );

    _server.begin();
  } else {
    Serial.println("\nVerbindung fehlgeschlagen. Starte AP-Modus.");
    _config->configured = false;
    _startAP();
  }
}

void WifiConfigManager::_setupMDNS() {
  if (MDNS.begin(_config->mdns)) {
    Serial.println("mDNS-Responder gestartet.");
    MDNS.addService("http", "tcp", 80);
  } else {
    Serial.println("Fehler beim Starten des mDNS-Responders!");
  }
}

// ---- MQTT-Helfer ----
void WifiConfigManager::_reconnectMQTT() {
  if (String(_config->mqttIp).length() == 0) return; // optional
  _mqttClient.setServer(_config->mqttIp, _config->mqttPort);
  if (_mqttClient.connected()) return;

  uint64_t chipId = ESP.getEfuseMac();
  char cid[64];
  snprintf(cid, sizeof(cid), "%s-%04X", _config->mdns, (uint16_t)(chipId >> 32));

  if (String(_config->mqttUser).length() > 0) {
    _mqttClient.connect(cid, _config->mqttUser, _config->mqttPasswd);
  } else {
    _mqttClient.connect(cid);
  }
}

bool WifiConfigManager::ensureMqttConnected() {
  if (!isWifiConnected()) return false;
  if (!_mqttClient.connected()) _reconnectMQTT();
  return _mqttClient.connected();
}

bool WifiConfigManager::publish(const char* topic, const String& payload, bool retain, int /*qos*/) {
  if (!ensureMqttConnected()) return false;
  return _mqttClient.publish(topic, payload.c_str(), retain);
}

// ---- HTML & Form ----
String WifiConfigManager::_getHtmlForm() {
  String html;
  html += "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>Konfiguration</title>";
  html += "<style>body{font-family:Arial,sans-serif;margin:20px;background-color:#f4f4f4;color:#333;font-size:16px;}form{max-width:600px;margin:auto;background:white;padding:20px;border-radius:8px;box-shadow:0 0 10px rgba(0,0,0,0.1);}h1{font-size:2em;font-weight:bold;}h2{font-size:1.5em;font-weight:bold;}h3{font-size:1.17em;font-weight:bold;}hr{border:0;height:1px;background-color:#ccc;margin:20px 0;}.form-row{display:flex;align-items:center;margin-bottom:15px;}.form-row label{flex:1;padding-right:20px;text-align:right;white-space:nowrap;font-size:1em;}.form-row input[type='text'],.form-row input[type='password'],.form-row input[type='number']{flex:2;padding:8px;border:1px solid #ccc;border-radius:4px;font-size:1em;}.form-row .status-param{flex:2;padding:8px;background-color:#e9ecef;border:none;border-radius:4px;font-size:1em;}.form-row .required-input{background-color:#fff9e6;}.form-row .checkbox-container{flex:2;display:flex;align-items:center;}.form-row .checkbox-container input[type='checkbox']{margin-right:10px;}.config-block{border:1px solid #ddd;padding:15px;margin-bottom:20px;border-radius:4px;}.button-container{margin-top:30px;text-align:center;}.button-container button{padding:10px 20px;font-size:1.1em;cursor:pointer;background-color:#4CAF50;color:white;border:none;border-radius:5px;}.blank-line{height:2em;}</style></head><body><form action='/save' method='POST'>";

  for (int i = 0; i < _webFormCount; i++) {
    const WebStruc& element = _webForm[i];
    int extraIndex = _findExtraParamIndex(element.relatedKey);
    switch (element.lineType) {
      case TITLE:       html += "<h1>" + String(element.label) + "</h1>"; break;
      case CONFIGBLOCK:
        html += "<div class='config-block'><h2>WLAN und MQTT Konfiguration</h2><h3>WLAN Einstellungen:</h3>";
        html += "<div class='form-row'><label for='ssid'>SSID:</label><input type='text' id='ssid' name='ssid' value='" + String(_config->ssid) + "' class='required-input' required></div>";
        html += "<div class='form-row'><label for='ssidpasswd'>Passwort:</label><input type='password' id='ssidpasswd' name='ssidpasswd' value='" + String(_config->ssidpasswd) + "' class='required-input' required></div>";
        html += "<div class='form-row'><label></label><div class='checkbox-container'><input type='checkbox' id='show-ssidpasswd'><label for='show-ssidpasswd'>Passwort anzeigen</label></div></div>";
        html += "<div class='form-row'><label for='mdns'>mDNS Hostname:</label><input type='text' id='mdns' name='mdns' value='" + String(_config->mdns) + "' class='required-input' required></div>";
        html += "<h3>MQTT Einstellungen (optional):</h3>";
        html += "<div class='form-row'><label for='mqttIp'>Server:</label><input type='text' id='mqttIp' name='mqttIp' value='" + String(_config->mqttIp) + "'></div>";
        html += "<div class='form-row'><label for='mqttPort'>Port:</label><input type='number' id='mqttPort' name='mqttPort' value='" + String(_config->mqttPort) + "'></div>";
        html += "<div class='form-row'><label for='mqttUser'>Benutzername:</label><input type='text' id='mqttUser' name='mqttUser' value='" + String(_config->mqttUser) + "'></div>";
        html += "<div class='form-row'><label for='mqttPasswd'>Passwort:</label><input type='password' id='mqttPasswd' name='mqttPasswd' value='" + String(_config->mqttPasswd) + "'></div>";
        html += "<div class='form-row'><label></label><div class='checkbox-container'><input type='checkbox' id='show-mqttPasswd'><label for='show-mqttPasswd'>Passwort anzeigen</label></div></div>";
        html += "</div>";
        break;
      case SEPARATOR:   html += "<hr>"; break;
      case BLANK:       html += "<div class='blank-line'></div>"; break;
      case PARAMETER:
        if (extraIndex != -1) {
          ExtraStruc& param = _extraParams[extraIndex];
          html += "<div class='form-row'><label for='" + String(param.keyName) + "'>" + String(element.label) + ":</label>";
          if (param.inputParam) {
            String inputClass = param.optional ? "" : "required-input";
            String requiredAttr = param.optional ? "" : "required";
            switch (param.formType) {
              case STRING:
                html += "<input type='text' id='" + String(param.keyName) + "' name='" + String(param.keyName) + "' value='" + String(param.TEXTvalue) + "' class='" + inputClass + "' " + requiredAttr + ">"; break;
              case FLOAT:
                html += "<input type='number' id='" + String(param.keyName) + "' name='" + String(param.keyName) + "' value='" + String(param.FLOATvalue,1) + "' step='any' class='" + inputClass + "' " + requiredAttr + ">"; break;
              case LONG:
                html += "<input type='number' id='" + String(param.keyName) + "' name='" + String(param.keyName) + "' value='" + String(param.LONGvalue) + "' class='" + inputClass + "' " + requiredAttr + ">"; break;
              case BOOL:
                html += String("<div class='checkbox-container'><input type='checkbox' id='") + String(param.keyName) + "' name='" + String(param.keyName) + "'" + (param.BOOLvalue?" checked":"") + "></div>"; break;
            }
          } else {
            // Statusausgabe
            switch (param.formType) {
              case STRING: html += "<span class='status-param'>" + String(param.TEXTvalue) + "</span>"; break;
              case FLOAT:  html += "<span class='status-param'>" + String(param.FLOATvalue,1) + "</span>"; break;
              case LONG:   html += "<span class='status-param'>" + String(param.LONGvalue) + "</span>"; break;
              case BOOL:   html += String("<div class='checkbox-container'><input type='checkbox' id='") + String(param.keyName) + "' name='" + String(param.keyName) + "' disabled" + (param.BOOLvalue?" checked":"") + "></div>"; break;
            }
          }
          html += "</div>";
        }
        break;
      default: break;
    }
  }

  html += "<hr>";
  html += "<div class='form-row'><label></label><div class='checkbox-container'><input type='checkbox' id='reset_config' name='reset_config'><label for='reset_config'>Alle Konfigurationsdaten löschen (Werkseinstellung!)</label></div></div>";
  html += "<div class='button-container'><button type='submit'>Daten übernehmen</button></div>";
  html += "</form>";
  html += "<script>function togglePass(passId,cb){var p=document.getElementById(passId);var c=document.getElementById(cb);p.type=c.checked?'text':'password';}document.getElementById('show-ssidpasswd').addEventListener('change',function(){togglePass('ssidpasswd','show-ssidpasswd')});var m=document.getElementById('show-mqttPasswd');if(m){m.addEventListener('change',function(){togglePass('mqttPasswd','show-mqttPasswd')});}</script>";
  html += "</body></html>";
  return html;
}

bool WifiConfigManager::_validateForm(AsyncWebServerRequest* request) {
  if (request->hasArg("reset_config")) {
    Serial.println("An dieser Stelle wird in _validateForm der NVS gelöscht :-(");
    _prefsNetwork.begin  (PREFS_NAMESPACE_NETWORK,   false); _prefsNetwork.clear(); _prefsNetwork.end();
    _prefsOperation.begin(PREFS_NAMESPACE_OPERATION, false); _prefsOperation.clear(); _prefsOperation.end();
    ESP.restart();
    return false;
  }

  String errorList;
  String ssid = request->arg("ssid");
  if (ssid.length() == 0) errorList += "<li>SSID ist ein Pflichtfeld.</li>";

  for (int i = 0; i < _anzExtraparams; i++) {
    ExtraStruc& param = _extraParams[i];
    if (param.inputParam) {
      if (param.formType == BOOL) { param.BOOLvalue = request->hasArg(param.keyName); continue; }
      String val = request->arg(param.keyName);
      if (!param.optional && val.length() == 0) {
        errorList += "<li>" + String(param.keyName) + " ist ein Pflichtfeld.</li>";
      }
      switch (param.formType) {
        case FLOAT: if (val.length()>0) { val.replace(',', '.'); param.FLOATvalue = val.toFloat(); } break;
        case LONG:  if (val.length()>0) { param.LONGvalue  = val.toInt(); } break;
        case BOOL:  break;
        default:    strncpy(param.TEXTvalue, val.c_str(), sizeof(param.TEXTvalue)); break;
      }
    }
  }

  if (errorList.length() > 0) {
    request->send(400, "text/html", _getValidationErrorHtml(errorList));
    return false;
  }

  strncpy(_config->ssid,      request->arg("ssid").c_str(),      sizeof(_config->ssid));
  strncpy(_config->ssidpasswd,request->arg("ssidpasswd").c_str(), sizeof(_config->ssidpasswd));
  strncpy(_config->mdns,      request->arg("mdns").c_str(),      sizeof(_config->mdns));
  strncpy(_config->mqttIp,    request->arg("mqttIp").c_str(),    sizeof(_config->mqttIp));
  _config->mqttPort = request->arg("mqttPort").toInt();
  strncpy(_config->mqttUser,  request->arg("mqttUser").c_str(),  sizeof(_config->mqttUser));
  strncpy(_config->mqttPasswd,request->arg("mqttPasswd").c_str(),sizeof(_config->mqttPasswd));
  return true;
}

String WifiConfigManager::_getValidationErrorHtml(const String& errorList) {
  return String(F("<html><head><meta charset='UTF-8'></head><body>")) +
         F("<h2>Eingaben fehlerhaft</h2><p>Bitte korrigiere folgende Felder:</p><ul>") +
         errorList + F("</ul><button onclick='history.back()'>Zurück</button></body></html>");
}

int WifiConfigManager::_findExtraParamIndex(const char* keyName) {
  for (int i = 0; i < _anzExtraparams; i++) {
    if (strcmp(_extraParams[i].keyName, keyName) == 0) return i;
  }
  return -1;
}
