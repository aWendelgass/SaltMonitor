# Protoyp Status 
Dieses Projekt verwende ich mit einem AQMOS R2D2 32 Wasserenthärter.
Das ganze Gerät steht auf einer speziellen 3D Gedruckten Waage. Dies ist über ein gewöhnliches Netzwerkkabel  mit einer Controlbox verbunden. Darin sitzt ein ESP 32.
Momentan werden alle Daten an einen MQTT Server (in IOBROKER) übertragen. Momentan ist das ganze ein Messprototyp. Ziel ist es das zu einem autarken System auszubauen, 
welches an das Handy bzw. an IOBROKER meldet, wenn Regeneriersalz nachgefüllt werden muss 
  
I am using this project with an AQMOS R2D2 32 water softener.
The whole device stands on a special 3D printed scale. This is connected to a control box via an ordinary network cable. Inside is an ESP 32.
All data is currently transmitted to an MQTT server (in IOBROKER). At the moment the whole thing is a measurement prototype. The aim is to develop it into a self-sufficient system,
which reports to the mobile phone or IOBROKER when regeneration salt needs to be topped up

# Salt Monitor for Water Softener

This project is an Arduino-based firmware for an ESP32 to monitor the salt level in a water softener by continuously weighing it. It provides a web interface for configuration, an OLED display for live status, and integrates with home automation systems via MQTT.

The original code comments and on-screen display text are in German.

## Features

- **Weight Sensing:** Measures weight using an HX711 load cell amplifier and a standard load cell.
- **OLED Display:** Shows the current weight, calibration status, and an interactive menu on a 128x64 SSD1306 OLED display.
- **Web-Based Configuration:** If not configured, the device starts a WiFi Access Point (AP). A web page allows you to set up WiFi, MQTT credentials, and other parameters.
- **MQTT Integration:** Publishes the salt weight and device status to an MQTT broker, making it easy to integrate with platforms like Home Assistant, openHAB, or Node-RED.
- **On-Device Controls:** A single button is used to navigate a simple menu for taring the scale, starting the calibration process, and viewing device information.
- **Persistent Storage:** All configuration is saved to the ESP32's non-volatile storage and is retained after a reboot or power loss.
- **mDNS Discovery:** The device can be discovered on the local network via its mDNS hostname (e.g., `myESP.local`).

## Hardware Requirements

- ESP32 Development Board
- HX711 Load Cell Amplifier
- Load Cell (e.g., 50kg capacity, depending on your salt container)
- SSD1306 I2C OLED Display (128x64 pixels)
- Tactile Push Button
- 5V Power Supply
- (Optional) An LED for status indication.

### Default Pinout

The firmware is configured for the following pin connections. This can be changed in `SaltMonitor.ino`.

| Component      | Pin Name     | ESP32 Pin |
|----------------|--------------|-----------|
| HX711          | `DOUT`       | 25        |
| HX711          | `SCK`        | 27        |
| OLED Display   | `SDA`        | 21        |
| OLED Display   | `SCL`        | 22        |
| Control Button | `BUTTON_PIN` | 13        |
| Status LED     | `LED_PIN`    | 17        |


## Software & Libraries

This project is built using the Arduino framework for the ESP32. You will need to install the following libraries through the Arduino Library Manager:

- `HX711_ADC` by olkal
- `Adafruit GFX Library` by Adafruit
- `Adafruit SSD1306` by Adafruit
- `U8g2_for_Adafruit_GFX` by olikraus
- `ESPAsyncWebServer`
- `AsyncTCP`
- `PubSubClient` by Nick O'Leary

## Initial Setup & Configuration

On the first boot, or after a factory reset, the device cannot connect to a WiFi network and will enter AP (Access Point) mode.

1.  The device will create a new WiFi network with a name like **`myESP-Setup-XXXX`**.
2.  Connect your computer or smartphone to this WiFi network.
3.  Open a web browser and navigate to **`http://192.168.4.1`**.
4.  The web configuration portal will load. Here you can set:
    - **WLAN Settings:** Your home WiFi network's SSID and password.
    - **mDNS Hostname:** A local network name for the device (e.g., `salz-monitor`).
    - **MQTT Settings:** The IP address, port, and credentials for your MQTT broker (optional).
    - **Application Parameters:**
        - `Kalibrierungsgewicht (kg)`: The known weight (in kilograms) you will use for calibration.
        - `Akkustischer Alarm`: (Not implemented in this version)
        - `Initialer Salz-Füllstand`: (Not implemented in this version)
5.  Click **"Daten übernehmen"** (Apply Data) to save the configuration. The device will reboot and attempt to connect to your configured WiFi network.

## Operation

Once configured, the device will display the current weight on the OLED screen. The single button allows you to perform several actions based on the duration of the press.

- **Short Press (< 0.5s):** Enters the menu and cycles through the pages: `LIVE` -> `TARE` -> `CALIBRATION` -> `INFO` -> `RESET`.
- **Medium Press (2-5s):** When the `TARE` page is active, this will tare the scale (set the current weight to zero). This is useful after placing the empty salt container on the scale.
- **Long Press (5-10s):** When the `CALIBRATION` page is active, this will start the interactive calibration process.
- **Very Long Press (> 10s):** Triggers a factory reset. All saved configuration will be erased, and the device will restart in AP mode.

### Calibration Process

To get accurate readings, you must calibrate the scale.

1.  Enter the known weight you will use for calibration in the web interface (e.g., `5.0` for a 5kg weight).
2.  Navigate to the `CALIBRATION` page using short presses of the button.
3.  Press and hold the button for 5-10 seconds to begin.
4.  Follow the on-screen prompts:
    1.  `Alles runternehmen` (Remove everything from the scale). Press the button.
    2.  `Lege Kalibriergewicht auf` (Place the calibration weight on the scale). Press the button.
5.  The scale is now calibrated. The new calibration factor is saved automatically.

## MQTT Integration

The device publishes data to the MQTT broker, allowing integration with home automation systems. The base topic is derived from the **mDNS Hostname** you set during configuration.

- **Weight Topic:** `<base_topic>/gewicht_kg`
  - **Payload:** The current weight in kilograms, as a floating-point number (e.g., `12.7`).
  - This message is **retained**.

- **Calibration Status Topic:** `<base_topic>/calibrated`
  - **Payload:** `1` if the scale has been calibrated, `0` otherwise.
  - This message is **retained**.
