
#ifndef WAAGE_H
#define WAAGE_H

#include <Arduino.h>
#include <HX711_ADC.h>

// Struktur für Kalibrierdaten
struct KalibrierungsDaten {
  float kalibrierungsfaktor; // CalFactor der HX711_ADC-Bibliothek
  long  tareOffset;          // Tare-Offset der HX711_ADC-Bibliothek
  bool  istKalibriert;       // Flag, ob gültige Daten vorliegen
};

// Callback-Typ für Benutzerhinweise (z. B. OLED)
typedef void (*UiMsgFn)(const char* line1, const char* line2);

class Waage {
public:
  Waage(int doutPin, int sckPin, int tastenPin, int ledPin);

  // Start mit übergebenen Kalibrierdaten
  void begin(const KalibrierungsDaten& daten);

  // zyklisch aufrufen
  void loop();

  // Interaktive Kalibrierung mit bekanntem Referenzgewicht (in Gramm)
  KalibrierungsDaten kalibriereWaage(float kalibrierungsgewicht);

  // Anzeigeeinstellungen: Genauigkeit in Gramm (z. B. 100 -> 1 Nachkommastelle, 10 -> 2, 1 -> 3)
  void setAnzeigeGenauigkeitGramm(uint16_t genauigkeit_g);

  // Bedienfunktionen
  void tare(); // Tarieren (setzt aktuell aufgelegtes Gewicht auf 0)

  // UI-Callbacks setzen (für OLED-Hinweise in kalibriereWaage)
  void setUiCallback(UiMsgFn cb);

  // Getter
  float getGewicht();     // in der Kalibriereinheit (hier: Gramm)
  float getGewichtKg();   // in Kilogramm
  float getKalibrierungsfaktor();
  long  getTareOffset();
  bool  istKalibriert();

private:
  HX711_ADC          _loadCell;
  float              _lastWeight;            // letzte Rohmessung (in g) – ungeglättet
  float              _emaWeight;             // geglätteter Wert (in g)
  bool               _emaInit;               // EMA initialisiert
  float              _lastPrintedKg;         // letzte ausgegebene, GERUNDETE kg
  bool               _hasLastPrinted;        // schon etwas ausgegeben
  bool               _hasLastOutput;         // Alt: verhindert Fluten
  int                _tastenPin;
  int                _ledPin;
  KalibrierungsDaten _daten;

  // Anzeigeformatierung
  uint16_t _anzeigeGenauigkeit_g;            // z. B. 100 g, 10 g, 1 g
  uint8_t  _anzeigeDezimalstellen;           // abgeleitet aus Genauigkeit

  // UI Callback
  UiMsgFn  _uiCb;

  void    _warteAufTasterDruck();            // erwartet Taster als INPUT_PULLUP (aktiv LOW)
  void    _blinkLED(int count, int delayMs);
  uint8_t _berechneDezimalstellen(uint16_t genauigkeit_g);
};

#endif
