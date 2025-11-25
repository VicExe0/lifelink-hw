# Lifelink hardware

# Usage
1. Power the HW-389 board via Barrel cabel 5mm 6V-24V.  (do not power ESP via USB only, USB connector is only for programming and may lead to issues with other components)
2. ESP will try to connect to WIFI 5 times (1-10s) and if it fails to then dev mode will be activated and light on the board will light up. (Dev mode will be activated if ESP loses connection for 5 seconds by default)
    - ESP will start a hotspot, connect to it and configure the wifi settings and card registration on IP `192.168.4.1` by default.
    + To register a card, place it on the RFID reader, input PESEL and press REGISTER CARD button below. If it fails, follow the steps on the page and try again. Reader could fail sometimes, in that case refresh the request page.
3. To turn OFF/ON the dev mode which disables the card reader (blue light visible on the board not the HW-389 board), hold button for 3 seconds.

## List
- ESP8266 NodeMCU Ver 0.1
- HW-389
- RFID-RC522
- Servo SG90 Positional
- Button

## Connections
- ESP8266 -> HW-389
- RFID-RC522:
    + SDA  -> D2
    + SCK  -> D5
    + MOSI -> D7
    + MISO -> D6
    + RQ   -> N/A
    + GND  -> GND
    + RST  -> D1
    + 3.3V -> 3V
- SG90 Servo:
    + VCC  -> 5V (on HW-389)
    + GND  -> GND
    + PWM  -> D0
- Button:
    + 3V   -> A0

## Instructions:
1. Install Arduino IDE version 2.3.6
2. Go to `File > Preferences > Additional boards menager URLs` and paste `https://arduino.esp8266.com/stable/package_esp8266com_index.json` then pres OK.
3. Go to `Tools > Board` and select `NodeMCU 1.0 (ESP-12E Module)` at your COM port.
4. Install dependencies:
    - MFRC522       v1.4.12
    - NDEF_MFRC522  v2.0.1  (Adafruit PN532/NfcAdapter)
    + other libraries are build-in into Arduino IDE or core libraries for ESP8266
5. Go to `Tools` and use this settings:
    - Upload Speed:     115200
    - Flash Size:       4MB (FS:2MB OTA:~1019KB)
    - CPU Frequency:    80 MHZ
    - Erase Flash:      Only Sketch

### NOTE:

If you are unable to upload to the ESP then try disconnecting cables from pins D0 and D3.
