#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <NfcAdapter.h>
#include <MFRC522.h>
#include <EEPROM.h>
#include <Servo.h>
#include <math.h>
#include <SPI.h>

#include "page.h"

#define RST_PIN D1
#define SS_PIN D2
#define BUTTON A0 // dev mode button
#define SERVO_PIN D0

Servo servo;
ESP8266WebServer server(80);
MFRC522 mfrc522(SS_PIN, RST_PIN);
NfcAdapter nfc(&mfrc522);

String ssid_mem = "";
String pass_mem = "";

const int MAX_WIFI_DOWNTIME = 5000;   // ms
const int BUTTON_DOWN_TRIGGER = 3000; // ms
const int START_POS_DEG = 180;

unsigned long pressStart = 0;
unsigned long wifiDownStart = 0;
bool dev_server_running = false;
bool button_last_state = false;
bool ignore_button = false;
bool dev_mode = false;
int current_deg = START_POS_DEG;

int requestGet( String& url, String& body ) {
  if ( WiFi.status() != WL_CONNECTED ) return -1;

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  if ( !http.begin(client, url) ) return -2;
  int code = http.GET();
  body = http.getString();
  http.end();

  return code;
}

int requestPostJson( String& url, String& response, String& payload ) {
  if ( WiFi.status() != WL_CONNECTED ) return -1;

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  if ( !http.begin(client, url) ) return -2;
  http.addHeader("Content-Type", "application/json");

  int code = http.POST(payload);
  response = http.getString();
  http.end();

  return code;
}

String readStr( int start ) {
  String s = "";
  char c;

  for (int i = start; i < start + 64; i++) {
    c = EEPROM.read(i);

    if (c == 0) break;

    s += c;
  }

  return s;
}

void writeStr( int start, const String& s ) {
  for ( int i = 0; i < s.length(); i++ ) EEPROM.write(start + i, s[i]);
  EEPROM.write(start + s.length(), 0);
}

void loadCreds() {
  ssid_mem = readStr(0);
  pass_mem = readStr(64);
}

void saveCreds( const String& s, const String& p ) {
  EEPROM.begin(512);
  writeStr(0, s);
  writeStr(64, p);
  EEPROM.commit();
}

void setServoPosition( int deg ) {
  if ( deg > 180 || deg < 0 ) return;

  servo.writeMicroseconds( ( deg * 2000 / 180 ) + 500 );
}

bool validUID( String uid ) {
  if ( WiFi.status() != WL_CONNECTED ) {
    Serial.println("Lost connection.");
    return false;
  }

  String url = "https://dom.optotel.pl/api/nfc/verify";
  String payload = "{\"uid\":\"" + uid + "\"}";

  String response;
  int code = requestPostJson(url, response, payload);

  Serial.print("Response status code: ");
  Serial.println(code);

  Serial.print("Response body: ");
  Serial.println(response);

  if ( code == 200 || response == "true" ) {
    return true;
  }

  return false;
}

bool registerCard( String pesel, String password ) {
  if ( WiFi.status() != WL_CONNECTED ) {
    Serial.println("Lost connection.");
    return false;
  }

  if ( !nfc.tagPresent() ) {
    int attempts = 0;

    for ( ; attempts < 3; attempts++ ) {
      if ( nfc.tagPresent() ) break;
      delay(100);
    }

    if ( attempts >= 3 ) {
      Serial.println("Card not present.");
      return false;
    }
  }

  String uidstr = uidToStr(mfrc522.uid);

  String url = "https://dom.optotel.pl/api/nfc/register";
  String payload = "{\"nfcTagUid\":\"" + uidstr + "\",\"pesel\":\"" + pesel +"\",\"password\":\"" + password + "\"}";

  Serial.println(payload);

  String response;
  int code = requestPostJson(url, response, payload);

  Serial.print("Response status code: ");
  Serial.println(code);

  Serial.print("Response body: ");
  Serial.println(response);

  if ( code != 200 ) {
    Serial.print("Request failed");
    return false;
  }

  return true;
}

String uidToStr( MFRC522::Uid uid ) {
    String out = "";

    for ( byte i = 0; i < uid.size; i++ ) {
        if ( uid.uidByte[i] < 16 ) out += "0";

        out += String(uid.uidByte[i], HEX);

        if ( i != uid.size - 1 ) out += ":";
    }

    out.toUpperCase();
    return out;
}

String getPrefix( byte code ) {
  switch ( code ) {
    case 0x01: return "http://www.";
    case 0x03: return "http://";
    case 0x04: return "https://www.";

    default: return "";
  }
}

String getRecordType( NdefRecord& record ) {
  const byte* typeBytes = record.getType();
  int typeLen = record.getTypeLength();

  String type = "";
  for ( int i = 0; i < typeLen; i++ ) {
    type += (char)typeBytes[i];
  }

  return type;
}

void handleMessage( NdefMessage& msg ) {
  for ( int i = 0; i < msg.getRecordCount(); i++ ) {
    NdefRecord record = msg.getRecord(i);

    String type = getRecordType(record);

    const byte* payload = record.getPayload();
    int length = record.getPayloadLength();

    Serial.print("Record ");
    Serial.print(i);
    Serial.print(": ");

    if ( type == "T" ) {
      int status = payload[0];
      int langLength = status & 0x3F;

      Serial.print("TEXT | ");
      for ( int j = 1 + langLength; j < length; j++ ) {
        Serial.print( (char)payload[j] );
      }

      Serial.println();
    }
        
    else if ( type == "U" ) {
      byte code = payload[0];

      Serial.print("URL | ");
      String prefix = getPrefix(code);

      Serial.print(prefix);
      for ( int j = 1; j < length; j++ ) {
        Serial.print( (char)payload[j] );
      }

      Serial.println();
    }
        
    else {
      Serial.print("Unregistered record type: "); 
      Serial.println(type);
    }
  }
}

bool buttonPressed() {
  return analogRead(BUTTON) >= 1000;
}

void setDevMode( bool state ) {
  if ( dev_mode == state ) return;

  if ( state ) {
    dev_mode = true;
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("DEVMODE ON.");

  } else {
    dev_mode = false;
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("DEVMODE OFF.");
  }
}

void checkDevMode() {
  if ( !dev_mode && WiFi.status() != WL_CONNECTED ) {
    if ( wifiDownStart == 0 ) wifiDownStart = millis();

    if ( millis() - wifiDownStart >= MAX_WIFI_DOWNTIME ) {
      setDevMode(true);
    }
  }

  if ( !ignore_button && buttonPressed() && button_last_state ) {
    if ( pressStart == 0 ) pressStart = millis();

    if ( millis() - pressStart >= BUTTON_DOWN_TRIGGER ) {
      setDevMode(!dev_mode);
      ignore_button = true;
      pressStart = 0;
    }
  }

  if ( !buttonPressed() ) {
    pressStart = 0;
    button_last_state = false;
    ignore_button = false;

  } else {
    button_last_state = true;
  }
}

bool attemptToConnect() {
  delay(2000);

  if ( WiFi.status() == WL_CONNECTED ) return true;
  
  for ( int i = 0; i < 5; i++ ) {
    Serial.print("Reconnecting... Attempt: ");
    Serial.print(i + 1);
    Serial.println("/5");

    if ( WiFi.status() == WL_CONNECTED ) return true;

    delay(2000);
  }

  return false;
}

void handleCard() {
  NfcTag tag = nfc.read();

  MFRC522::Uid uid = mfrc522.uid;
  String uidstr = uidToStr(uid);
  Serial.print("UID: ");
  Serial.println(uidstr);

  if ( tag.hasNdefMessage() ) {
    NdefMessage msg = tag.getNdefMessage();
    handleMessage(msg);
    
  } else {
    Serial.println("No NDEF payload on tag");
  }

  if ( validUID(uidstr) ) {
    current_deg += -45;
    setServoPosition(current_deg);
  }

  delay(1000);
}

void startDevServer() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("ESP_SETUP");

  server.on("/", []() {
    String page = FPSTR(dashboard);
    page.replace("{ssid}", ssid_mem);
    page.replace("{password}", pass_mem);
    server.send(200, "text/html", page);
  });

  server.on("/save", []() {
    String arg_ssid = server.arg("ssid");
    String arg_password = server.arg("password");

    saveCreds(arg_ssid, arg_password);

    WiFi.disconnect(true);
    delay(200);

    WiFi.begin(arg_ssid.c_str(), arg_password.c_str());
    server.send(200, "text/html", "Saved");
  });

  server.on("/register", []() {
    String arg_pesel = server.arg("pesel");
    String arg_password = server.arg("password");

    if ( registerCard(arg_pesel, arg_password) ) {
      server.send(200, "text/html", "Success!");

    } else {
      server.send(200, "text/html", "Failed. Make sure that the PESEL is correct, WiFi is configured & connected and card is on the reader.");
    }
  });

  server.begin();
  dev_server_running = true;
}

void stopDevServer() {
  server.stop();
  WiFi.softAPdisconnect(true);
  dev_server_running = false;
}


void setup() {
  EEPROM.begin(512);
  loadCreds();
  SPI.begin();
  Serial.begin(115200);

  mfrc522.PCD_Init();
  nfc.begin();

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(SERVO_PIN, OUTPUT);

  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(SERVO_PIN, LOW);

  servo.attach(SERVO_PIN, 500, 2500);
  setServoPosition(START_POS_DEG);

  if ( ssid_mem.length() == 0 ) setDevMode(true);

  else {
    WiFi.begin(ssid_mem.c_str(), pass_mem.c_str());

    delay(1000);

    if ( !attemptToConnect() ) {
      Serial.println("Failed to connect to Wifi. Dev mode activated.");
      setDevMode(true);
    }
  }
}

void loop() {
  checkDevMode();

  if ( dev_mode && !dev_server_running ) startDevServer();
  if ( !dev_mode && dev_server_running ) stopDevServer();

  if ( dev_server_running ) server.handleClient();

  if ( !dev_mode && nfc.tagPresent() ) handleCard();
  
  delay(100);
}
