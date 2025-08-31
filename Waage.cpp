
#include "Waage.h"
#include <math.h>

// Optional: Parameter für Ausgabe/Update
static const int   UPDATE_INTERVAL_MS       = 500;
static const float OUTPUT_TOLERANCE_PERCENT = 5.0f;
static const float EMA_ALPHA                = 0.3f; // Glättung (0..1)

Waage::Waage(int doutPin, int sckPin, int tastenPin, int ledPin)
: _loadCell(doutPin, sckPin),
  _lastWeight(0.0f),
  _emaWeight(0.0f),
  _emaInit(false),
  _lastPrintedKg(0.0f),
  _hasLastPrinted(false),
  _hasLastOutput(false),
  _tastenPin(tastenPin),
  _ledPin(ledPin),
  _anzeigeGenauigkeit_g(100),          // Default: 100 g (1 Nachkommastelle in kg)
  _anzeigeDezimalstellen(1),
  _uiCb(nullptr)
{}

void Waage::begin(const KalibrierungsDaten& daten) {
  _daten = daten;

  pinMode(_ledPin, OUTPUT);
  digitalWrite(_ledPin, LOW);

  _loadCell.begin();
  _loadCell.setSamplesInUse(16);
  _loadCell.start(2000); // Vorwärmzeit

  if (_daten.istKalibriert) {
    _loadCell.setCalFactor(_daten.kalibrierungsfaktor);
    _loadCell.setTareOffset(_daten.tareOffset);
    Serial.println(F("Waage mit geladenen Daten initialisiert."));
  } else {
    Serial.println(F("Waage unkalibriert. Benötigt Kalibrierung."));
  }

  // Reset Tracking
  _hasLastOutput  = false;
  _hasLastPrinted = false;
  _lastWeight     = 0.0f;
  _emaInit        = false;
}

void Waage::loop() {
  static unsigned long lastUpdate = 0;
  _loadCell.update();

  const unsigned long now = millis();
  if (now - lastUpdate >= UPDATE_INTERVAL_MS) {
    if (_daten.istKalibriert) {
      float gewicht_g = _loadCell.getData();

      // Plausibilitätscheck / Fehlerbehandlung
      if (isnan(gewicht_g) || gewicht_g < -100.0f) {
        Serial.println(F("Fehlerhafte Messung erkannt. Sensor wird neu gestartet."));
        _loadCell.start(2000);
        lastUpdate = now;
        return;
      }

      // Glättung (EMA)
      if (!_emaInit) { _emaWeight = gewicht_g; _emaInit = true; }
      else { _emaWeight = EMA_ALPHA * gewicht_g + (1.0f - EMA_ALPHA) * _emaWeight; }

      // Signifikanz-Logik: Prozentänderung ODER absolute Schwelle (halbe Anzeige-Genauigkeit)
      const float absDelta_g    = fabsf(_emaWeight - _lastWeight);
      const float pctChange     = (_hasLastOutput && fabsf(_lastWeight) > 0.0f)
                                ? (absDelta_g / fabsf(_lastWeight)) * 100.0f
                                : 0.0f;
      const float absThreshold_g = max(1.0f, _anzeigeGenauigkeit_g * 0.5f); // mind. 1 g

      bool significant = false;
      if (!_hasLastOutput) {
        significant = true; // erste Ausgabe
      } else if (fabsf(_lastWeight) > 0.0f) {
        significant = (pctChange >= OUTPUT_TOLERANCE_PERCENT) || (absDelta_g >= absThreshold_g);
      } else {
        significant = (absDelta_g >= absThreshold_g);
      }

      if (significant) {
        _lastWeight    = _emaWeight;
        _hasLastOutput = true;

        // Ausgabe nur, wenn sich die GERUNDETE Anzeige ändert (verhindert 0.0-Fluten)
        float kg           = _emaWeight / 1000.0f;
        float factor       = powf(10.0f, _anzeigeDezimalstellen);
        float roundedKg    = roundf(kg * factor) / factor;
        if (!_hasLastPrinted || fabsf(roundedKg - _lastPrintedKg) > (0.5f / factor)) {
          _lastPrintedKg   = roundedKg;
          _hasLastPrinted  = true;
          Serial.print(F("Gewicht: "));
          Serial.print(roundedKg, _anzeigeDezimalstellen);
          Serial.println(F(" kg"));
        }
      }
    } else {
      Serial.println(F("Waage nicht kalibriert. Warte auf Kalibrierung."));
    }
    lastUpdate = now;
  }
}

KalibrierungsDaten Waage::kalibriereWaage(float kalibrierungsgewicht) {
  KalibrierungsDaten neueDaten{};

  if (_uiCb) _uiCb("Alles runternehmen", "Taste drücken …");
  Serial.println(F("Nimm alles von der Waage. Drücke den Taster, um fortzufahren."));
  _warteAufTasterDruck();

  _loadCell.tare();
  _blinkLED(3, 100);
  if (_uiCb) _uiCb("Tare abgeschlossen", "Taste drücken …");
  Serial.println(F("Tare abgeschlossen."));

  char line2[32];
  snprintf(line2, sizeof(line2), "Gewicht %.2f g", kalibrierungsgewicht);
  if (_uiCb) _uiCb("Lege Kalibriergewicht", line2);
  Serial.print(F("Lege nun das Kalibrierungsgewicht ("));
  Serial.print(kalibrierungsgewicht, 2);
  Serial.println(F(" g) auf. Drücke den Taster, um fortzufahren."));
  _warteAufTasterDruck();

  _loadCell.refreshDataSet();
  neueDaten.kalibrierungsfaktor = _loadCell.getNewCalibration(kalibrierungsgewicht);
  neueDaten.tareOffset          = _loadCell.getTareOffset();
  neueDaten.istKalibriert       = true;

  // interne Daten aktualisieren
  _loadCell.setCalFactor(neueDaten.kalibrierungsfaktor);
  _loadCell.setTareOffset(neueDaten.tareOffset);
  _daten = neueDaten;

  // Ausgabe-Tracking zurücksetzen
  _hasLastOutput  = false;
  _hasLastPrinted = false;
  _lastWeight     = 0.0f;
  _emaInit        = false;

  if (_uiCb) _uiCb("Kalibrierung fertig", "");
  Serial.print(F("Kalibrierung abgeschlossen. Faktor: "));
  Serial.println(neueDaten.kalibrierungsfaktor, 5);
  Serial.print(F("Tare Offset: "));
  Serial.println(neueDaten.tareOffset);

  return neueDaten;
}

void Waage::setAnzeigeGenauigkeitGramm(uint16_t genauigkeit_g) {
  if (genauigkeit_g == 0) genauigkeit_g = 1; // Schutz
  _anzeigeGenauigkeit_g  = genauigkeit_g;
  _anzeigeDezimalstellen = _berechneDezimalstellen(genauigkeit_g);
}

void Waage::tare() {
  _loadCell.tare();
  _hasLastOutput  = false; // nächste Ausgabe wieder zulassen
  _hasLastPrinted = false;
  _lastWeight     = 0.0f;
  _emaInit        = false;
  Serial.println(F("Tare durchgeführt."));
}

void Waage::setUiCallback(UiMsgFn cb) { _uiCb = cb; }

float Waage::getGewicht() {
  _loadCell.update();
  return _loadCell.getData(); // Gramm (wenn in g kalibriert)
}

float Waage::getGewichtKg() { return getGewicht() / 1000.0f; }
float Waage::getKalibrierungsfaktor() { return _daten.kalibrierungsfaktor; }
long  Waage::getTareOffset() { return _loadCell.getTareOffset(); }
bool  Waage::istKalibriert() { return _daten.istKalibriert; }

void Waage::_warteAufTasterDruck() {
  // Taster: INPUT_PULLUP -> gedrückt = LOW
  while (digitalRead(_tastenPin) == HIGH) { delay(10); } // warten bis gedrückt
  while (digitalRead(_tastenPin) == LOW)  { delay(10); } // warten bis losgelassen
}

void Waage::_blinkLED(int count, int delayMs) {
  for (int i = 0; i < count; ++i) {
    digitalWrite(_ledPin, HIGH);
    delay(delayMs);
    digitalWrite(_ledPin, LOW);
    delay(delayMs);
  }
}

uint8_t Waage::_berechneDezimalstellen(uint16_t genauigkeit_g) {
  // Gramm-Genauigkeit -> Nachkommastellen in kg
  // 1000 g -> 0, 100 g -> 1, 10 g -> 2, 1 g -> 3 (clamp 0..3)
  if      (genauigkeit_g >= 1000) return 0;
  else if (genauigkeit_g >= 100)  return 1;
  else if (genauigkeit_g >= 10)   return 2;
  else                            return 3; // 1 g oder feiner
}
