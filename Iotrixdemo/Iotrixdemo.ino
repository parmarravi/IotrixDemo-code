/*This program  Controls Two led strip's over the APP with json data
  this program also handles Local Wifi ,OTP and Dynamic Wifi Connectivity with App.

  Features:
  1.)Control blink,Fade pattern for two led strips from the App
  2.)Ap mode ,Wifi Mode and Otp mode integrated
  3.)Communication is over TCP protocol
  4.)The device MAC Address is compare with saved mac address and vendor name for connectivity.
  5.)Dynamic way to change wifi and AP mode credential from APP

  //TODO:
  1)Need to  add SSDP  protocol for brodcasting unique device information ,this layer will help us to identify devices over network
  as host name is not working.THe verification of esp devices is done with unique networking layer in App itself.
  2)Adding security layer.

  //json data

  {
  "LedActivity":{
  "Type of Light":2,
  "Light Brightness":780,
  "TimeDelay":4050,
  "Blink":1,
  "Fade":1
  }
  }
  // wifi json
  {
  "WCon": true,
    "WSsid":"ssid_name",
    "WPwd":"ssid_pwd"
  }
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>

const int GreenLed = 0;    //setting up led strip pin
const int WhiteLed = 2;    //setting up led strip pin

ESP8266WebServer server(80); // Initializing tcp port

String Jsondata;
boolean getNet = false;

//Ap mode wifi credentials
const char* Localssid = "IOTRIX" ;   // default AP mode ssid
const char* Localpwd = "12345678" ;       //default AP mod password

//Wifi Credentials over the router
const char* WSSID ;
const char* WPWD ;

//Delcaring some Constants
int fadeAmount = 5;    // how many points to fade the LED by
int brightness = 0;    // how bright the LED is...
int LightT = 2;
int lightBrightness = 1023;
int LB1 = 0;
int TimeD = 0;

boolean blinkState = true;
boolean fadeState = false;
int WIFI = 0;
int Lwifi = 0;
// EEPROM ADDRESS FOR DATA STORAGE.
const int ADD_LEDBRIGHTNESS = 5;
const int ADD_BlinkState = 6;
const int ADD_FadeState = 7;
const int ADD_LightT  = 8;
const int ADD_TimeD  = 9;
const int ADD_WIFISTATE = 10;
const int ADD_LWIFISTATE = 11;

const int ADD_SSID_SIZE = 12;
const int ADD_PWD_SIZE = 13;

const int ADD_LSSID_SIZE = 14;
const int ADD_LPWD_SIZE = 15;

const int ADD_SSID = 16;
const int ADD_PWD = 50;

const int ADD_LSSID = 70;
const int ADD_LPWD = 100;

void setup() {
  EEPROM.begin(4000);    //4000 indicates memory size in esp12e change according to eeprom size for different esp models
  Serial.begin(9600);

  pinMode(GreenLed, OUTPUT);
  pinMode(WhiteLed, OUTPUT);

  //initial State of LED'S
  digitalWrite(GreenLed, HIGH);
  digitalWrite(WhiteLed, HIGH);

  //  //reading Data from EEprom
  LightT = EEPROM.read(ADD_LightT);
  Serial.print("LightT:");
  Serial.println(LightT);

  lightBrightness  =  (EEPROM.read(ADD_LEDBRIGHTNESS)) * 4 ;  //We can only store value from 0-255  to get value 1024 we will just multiply by 1024
  Serial.print("lightBrightness:");
  Serial.println(lightBrightness);

  TimeD =  (EEPROM.read(ADD_TimeD)) * 20;      //
  Serial.print("TimeD:");
  Serial.println(TimeD);

  blinkState = EEPROM.read(ADD_BlinkState);
  Serial.print("blinkState:");
  Serial.println(blinkState);

  fadeState = EEPROM.read(ADD_FadeState);
  Serial.print("fadeState:");
  Serial.println(fadeState);

  WIFI = EEPROM.read(ADD_WIFISTATE);

  Serial.println("wifi State");
  Serial.println(WIFI);
  Lwifi = EEPROM.read(ADD_LWIFISTATE);

  String www, sss;
  String Lww , Lss;

  if (Lwifi == 1) {

    for (int j = 0 ; j < EEPROM.read(ADD_LSSID_SIZE); j++) {
      Lww = Lww + char(EEPROM.read(ADD_LSSID + j ));
    }

    for (int j = 0 ; j < EEPROM.read(ADD_LPWD_SIZE) ; j++) {
      Lss =  Lss +  char(EEPROM.read(ADD_LPWD + j));
    }
    Localssid = Lww.c_str();  //conversion from string to const Char*, it was done due to esp not able to read char* this made esp memory leak
    Localpwd = Lss.c_str();   //conversion from string to const Char*, it was done due to esp not able to read char* this made esp memory leak
  }


  if (WIFI == 1) {
    for (int j = 0 ; j < EEPROM.read(ADD_SSID_SIZE); j++) {
      www = www + char(EEPROM.read(ADD_SSID + j ));
    }

    for (int j = 0 ; j < EEPROM.read(ADD_PWD_SIZE) ; j++) {
      sss =  sss +  char(EEPROM.read(ADD_PWD + j));
    }
    WSSID = www.c_str();  //conversion from string to const Char*, it was done due to esp not able to read char* this made esp memory leak
    WPWD = sss.c_str();   //conversion from string to const Char*, it was done due to esp not able to read char* this made esp memory leak

    if (ConnectToWifi()) {
      Serial.println("Connected to router");
    }
    else {
      Serial.println("Failed to Connect to router");
      LocalWifiConnect();    //failed to connect to wifi ,this shows that device is very far or router has been changed .
    }
  }
  else {
    LocalWifiConnect();       //default AP mode
  }

}

void loop() {
  server.handleClient();
  ArduinoOTA.handle();
  // put your main code here, to run repeatedly:
  if (getNet) {
    if (!ConnectToWifi())
    {
      Serial.println("error connecting");
      LocalWifiConnect();
      server.send (200, "text/plain", "Network failed to connect");
      getNet = false;
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    WiFi.softAPdisconnect(true);
  }
  Jsonlight(LightT, blinkState, fadeState, TimeD, lightBrightness);
}

void handleBody() { //Handler for the body path
  if (server.hasArg("plain") == false) { //Check if body received
    server.send(404, "text/plain", "error");
    return;
  }
  //  String message;
  //  message += server.arg("plain");
  //  message += "\n";
  DynamicJsonBuffer  jsonBuffer;
  String jsondata = urldecode(server.arg("plain"));
  Serial.println(jsondata);
  JsonObject& root = jsonBuffer.parseObject(jsondata);
  if (!root.success()) {
    server.send(404, "text/plain", "error");
  } else {
    server.send(200, "text/plain", "ok");
  }
  Serial.println(jsonBuffer.size());
  if (root.containsKey("WCon"))
  {
    boolean  WifiConnection = root["WCon"];
    server.send(200, "text/plain", WiFi.macAddress());
    if (!WifiConnection) {
      Serial.println("local");
      EEPROM.begin(4000);
      EEPROM.write(ADD_WIFISTATE, 0);
      EEPROM.end();
    }
    Serial.println(WifiConnection);
    if (WifiConnection) {
      Serial.println("connecting wifi..");
      getNet = true;
      const char* WSSIDs = root["WSsid"];
      const char* WPWDs = root["WPwd"];
      WSSID = strdup(WSSIDs);
      WPWD = strdup(WPWDs);
      ConnectToWifi();
    }
    else {
      LocalWifiConnect();
    }
  }

  if (root.containsKey("LCon"))
  {
    const char* LSSIDs = root["LSsid"];
    const char* LPWDs = root["LPwd"];
    Localssid = strdup(LSSIDs);
    Localpwd = strdup(LPWDs);
    EEPROM.begin(4000);
    EEPROM.write(ADD_LWIFISTATE, 1);
    EEPROM.write(ADD_LSSID_SIZE, strlen(Localssid));
    EEPROM.write(ADD_LPWD_SIZE, strlen(Localpwd));
    //WRITE SSID
    for (int j = 0 ; j < strlen(Localssid) ; j++) {
      EEPROM.write(ADD_LSSID + j, Localssid[j]);
    }
    //WRITE PWD
    for (int j = 0 ; j < strlen(Localpwd) ; j++) {
      EEPROM.write(ADD_LPWD + j, Localpwd[j]);
    }
    EEPROM.end();
  }
  if (root.containsKey("LedActivity")) {
    server.send(200, "text/plain", "Succeed");
    JsonObject& LedRoot = root["LedActivity"];
    LightT = LedRoot["Type of Light"];
    lightBrightness = LedRoot["Light Brightness"];
    Serial.println(lightBrightness);
    TimeD = LedRoot["TimeDelay"];
    blinkState = LedRoot["Blink"];
    fadeState = LedRoot["Fade"];
    //saving data to eeprom
    EEPROM.begin(4000);
    EEPROM.write(ADD_LightT, LightT);
    EEPROM.write(ADD_LEDBRIGHTNESS, (lightBrightness / 4)); //0-1024
    EEPROM.write(ADD_TimeD, (TimeD / 20)); //0-5100
    EEPROM.write(ADD_BlinkState, blinkState);
    EEPROM.write(ADD_FadeState, fadeState);
    EEPROM.end();
  }
  if (root.containsKey("OTA")) {
    server.send(200, "text/plain", "Succeed");
    boolean  wotaData = root["OTA"];
    if (wotaData) {
      OvertheAir();
    }
  }
}
boolean LocalWifiConnect() {
  Serial.println("LocalWifiConnect");
  WiFi.softAPdisconnect(false);
  boolean result = WiFi.softAP(Localssid, Localpwd);
  Serial.println(WiFi.localIP());
  server.on("/", handleBody);
  server.begin();
  getNet = false;
  return result;
}


//CHECK FOR WIFI CONNECTION SSID AND PWD WILL BE SEND FROM MOBILE PHONE USING JSON DATA
boolean ConnectToWifi() {

  Serial.println(WSSID);
  Serial.println(WPWD);

  WiFi.begin(WSSID, WPWD); // set ssid and pwd
  unsigned long startMillis = millis();
  unsigned long currentMillis = millis();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);// refer->
    Serial.print(".");
    currentMillis = millis();
    if ((currentMillis - startMillis) > 20000) {
      Serial.println("not able to conenct");
      EEPROM.begin(4000);
      EEPROM.write(ADD_WIFISTATE, 0);
      EEPROM.end();
      return false;  //check for 20 s for connection
      break;
    }
  }
  //Handle Successfull connection.
  if (WiFi.status() == WL_CONNECTED) {
    getNet = false;
    WiFi.softAPdisconnect(true);
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println (WiFi.localIP());
    server.on("/", handleBody);
    server.begin();
    Serial.println("Server started");

    //Save wifi credentials
    EEPROM.begin(4000);
    EEPROM.write(ADD_WIFISTATE, 1);
    EEPROM.write(ADD_SSID_SIZE, strlen(WSSID));
    EEPROM.write(ADD_PWD_SIZE, strlen(WPWD));

    //WRITE SSID
    for (int j = 0 ; j < strlen(WSSID) ; j++) {
      EEPROM.write(ADD_SSID + j, WSSID[j]);
    }
    //WRITE PWD
    for (int j = 0 ; j < strlen(WPWD) ; j++) {
      EEPROM.write(ADD_PWD + j, WPWD[j]);
    }
    EEPROM.end();
    return true;
  }
  return false;
}


void OvertheAir() {

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
void Jsonlight(int LType, boolean BlinkS, boolean fadeS, int Tdelay, int Lightness) {

  if (LType == 0) {
    if (BlinkS) {
      BlinkLight(GreenLed, Tdelay, Lightness);
    }
    else if (fadeS) {
      FadeLight(GreenLed);
    }
    BrightnessAdjustLight(GreenLed, Lightness);
    BrightnessAdjustLight(WhiteLed, 0);
    digitalWrite(WhiteLed, LOW);
  }
  if (LType == 1) {
    if (BlinkS)
    {
      BlinkLight(WhiteLed, Tdelay, Lightness);
    }
    if (fadeS)
    {
      FadeLight(WhiteLed);
    }
    BrightnessAdjustLight(WhiteLed, Lightness);
    BrightnessAdjustLight(GreenLed, 0);
    digitalWrite(GreenLed, LOW);
  }
  if (LType == 2) {
    BlinkLightAlernateLight(Tdelay, Lightness);
  }
  if (LType == 3) {
    if (BlinkS)
    {
      BlinkLight(Tdelay, Lightness);
    }
    else if (fadeS)
    {
      FadeLight();
    }
    else
    {
      BrightnessAdjustLight(Lightness);
    }
  }
}
void BlinkLight(int lightPin, int delayT, int brig) {
  analogWrite(lightPin, brig);
  delay(delayT);
  analogWrite(lightPin, 0);
  delay(delayT);
}
void BlinkLight(int delayT, int brig) {
  analogWrite(GreenLed, brig);
  analogWrite(WhiteLed, brig);
  delay(delayT);
  analogWrite(GreenLed, 0);
  analogWrite(WhiteLed, 0);
  delay(delayT);
}
void BlinkLightAlernateLight(int delayT, int brig) {
  analogWrite(GreenLed, brig);
  analogWrite(WhiteLed, 0);
  delay(delayT);
  analogWrite(GreenLed, 0);
  analogWrite(WhiteLed, brig);
  delay(delayT);
}
void BrightnessAdjustLight(int lightPin, int brig) {
  analogWrite(lightPin, brig);
}
void BrightnessAdjustLight(int brig) {
  analogWrite(GreenLed, brig);
  analogWrite(WhiteLed, brig);
}
void FadeLight(int lightPin) {
  for (int fadeValue = 1023 ; fadeValue >= 0; fadeValue -= 5) {
    analogWrite(lightPin, fadeValue);
    delay(10);
  }
  for (int fadeValue = 0 ; fadeValue <= 1023; fadeValue += 5) {
    analogWrite(lightPin, fadeValue);
    delay(10);
  }
}
void FadeLight() {
  for (int fadeValue = 1023 ; fadeValue >= 0; fadeValue -= 5) {
    analogWrite(GreenLed, fadeValue);
    analogWrite(WhiteLed, fadeValue);
    delay(10);
  }
  for (int fadeValue = 0 ; fadeValue <= 1023; fadeValue += 5) {
    analogWrite(GreenLed, fadeValue);
    analogWrite(WhiteLed, fadeValue);
    delay(10);
  }
}
String urldecode(String str)
{
  String encodedString = "";
  char c;
  char code0;
  char code1;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    //Serial.print(c);
   // Serial.print("......................................");
   // Serial.println(c,DEC);
    if (c == '+') {
     // Serial.println("plus detect");
      encodedString += ' ';
    } else if (c == '%') {
     // Serial.println("percent detect");
      i++;
      code0 = str.charAt(i);
      i++;
      code1 = str.charAt(i);
      c = (h2int(code0) << 4) | h2int(code1);
      encodedString += c;
    } else {
      encodedString += c;
    }

    yield();
  }

  return encodedString;
}

String urlencode(String str)
{
  String encodedString = "";
  char c;
  char code0;
  char code1;
  char code2;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == ' ') {
      encodedString += '+';
    } else if (isalnum(c)) {
      encodedString += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) {
        code0 = c - 10 + 'A';
      }
      code2 = '\0';
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
      //encodedString+=code2;
    }
    yield();
  }
  return encodedString;

}

unsigned char h2int(char c)
{
  if (c >= '0' && c <= '9') {
    return ((unsigned char)c - '0');
  }
  if (c >= 'a' && c <= 'f') {
    return ((unsigned char)c - 'a' + 10);
  }
  if (c >= 'A' && c <= 'F') {
    return ((unsigned char)c - 'A' + 10);
  }
  return (0);
}
