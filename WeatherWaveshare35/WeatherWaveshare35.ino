#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <ESP32Time.h>
#include <Preferences.h>
#include "board_init.h"
#include "NotoSansBold15.h"
#include "tinyFont.h"
#include "smallFont.h"
#include "midleFont.h"
#include "bigFont.h"
#include "font18.h"

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);
TFT_eSprite errSprite = TFT_eSprite(&tft);
ESP32Time rtc(0);
Preferences prefs;

// Author layout designed for 320x170 — scale to Waveshare 480x320 landscape
#define DESIGN_W 320
#define DESIGN_H 170
#define SCREEN_W 480
#define SCREEN_H 320
#define SX(v) ((int)((v) * SCREEN_W / DESIGN_W))
#define SY(v) ((int)((v) * SCREEN_H / DESIGN_H))
#define SW(v) ((int)((v) * SCREEN_W / DESIGN_W))
#define SH(v) ((int)((v) * SCREEN_H / DESIGN_H))
#define SR(v) ((int)((v) * SCREEN_W / DESIGN_W))

//#################### EDIT THIS  ###################
int zone = 7;
const float cityLat = 16.0544f;
const float cityLon = 108.2022f;
String town = "Da Nang";
// Leave empty and enter via WiFi setup portal, or paste your key here
String myAPI = "";
String units = "metric";
//#################### end of edits ###################

const char* ntpServer = "pool.ntp.org";
String server;

char apiKeyBuffer[48] = "";

int ani = SX(100);
unsigned long timePased = 0;
unsigned long lastDrawMs = 0;
int counter = 0;

#define DRAW_INTERVAL_MS 200

String lastClock = "";
String lastTicker = "";

#define bck TFT_BLACK
unsigned short grays[13];

const char* PPlbl[] = { "HUM", "PRESS", "WIND" };
const char* PPlblU[] = { "%", "hPa", "m/s" };

float temperature = 22.2;
float wData[3];
float PPpower[24] = {};
int PPgraph[24] = { 0 };
float maxT = 0;
float minT = 0;
bool historySeeded = false;

String Wmsg = "";

void initGrays();
void buildServer();
void loadApiKey();
void saveApiKey(const String& key);
void updateHistoryGraph(bool shiftSlot);
void getData();
void draw(bool force = false);

void loadApiKey() {
  prefs.begin("weather", true);
  String stored = prefs.getString("apikey", "");
  prefs.end();
  if (stored.length() > 0) {
    myAPI = stored;
  }
}

void saveApiKey(const String& key) {
  prefs.begin("weather", false);
  prefs.putString("apikey", key);
  prefs.end();
  myAPI = key;
}

void buildServer() {
  server = "https://api.openweathermap.org/data/2.5/weather?lat=" + String(cityLat, 4)
         + "&lon=" + String(cityLon, 4)
         + "&appid=" + myAPI + "&units=" + units;
}

void initGrays() {
  const uint8_t levels[] = {250, 220, 190, 160, 130, 100, 75, 55, 40, 28, 18, 12, 8};
  for (int i = 0; i < 13; i++) {
    uint8_t c = levels[i];
    grays[i] = tft.color565(c, c, c);
  }
}

void setTime() {
  configTime(3600 * zone, 0, ntpServer);
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    rtc.setTimeStruct(timeinfo);
  }
}

void setup() {
  Serial.begin(115200);
  buildServer();

  wsInitBoard();

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Connecting to WIFI!!", SX(30), SY(50), 4);

  sprite.createSprite(SCREEN_W, SCREEN_H);
  errSprite.createSprite(SW(164), SH(15));

  strncpy(apiKeyBuffer, myAPI.c_str(), sizeof(apiKeyBuffer) - 1);
  apiKeyBuffer[sizeof(apiKeyBuffer) - 1] = '\0';

  WiFiManager wifiManager;
  WiFiManagerParameter customApiKey(
    "apikey", "OpenWeather API Key", apiKeyBuffer, sizeof(apiKeyBuffer) - 1);
  wifiManager.addParameter(&customApiKey);
  wifiManager.setConfigPortalTimeout(5000);

  if (!wifiManager.autoConnect("VolosWifiConf", "password")) {
    Serial.println("Failed to connect and hit timeout");
    delay(3000);
    ESP.restart();
  }

  String newKey = customApiKey.getValue();
  if (newKey.length() >= 20) {
    saveApiKey(newKey);
    buildServer();
  }

  loadApiKey();
  buildServer();

  Serial.println("Connected.");
  setTime();
  getData();
  initGrays();
  tft.fillScreen(TFT_BLACK);
  draw(true);
}

void updateHistoryGraph(bool shiftSlot) {
  if (!historySeeded) {
    for (int i = 0; i < 24; i++) {
      PPpower[i] = temperature;
    }
    historySeeded = true;
  } else if (shiftSlot) {
    for (int i = 0; i < 23; i++) {
      PPpower[i] = PPpower[i + 1];
    }
    PPpower[23] = temperature;
  } else {
    PPpower[23] = temperature;
  }

  maxT = PPpower[0];
  minT = PPpower[0];
  for (int i = 1; i < 24; i++) {
    if (PPpower[i] > maxT) maxT = PPpower[i];
    if (PPpower[i] < minT) minT = PPpower[i];
  }

  for (int i = 0; i < 24; i++) {
    if (maxT - minT < 0.5f) {
      PPgraph[i] = 6;
    } else {
      PPgraph[i] = map((long)(PPpower[i] * 10), (long)(minT * 10), (long)(maxT * 10), 0, 12);
    }
  }
}

void getData() {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  if (!http.begin(client, server)) {
    Serial.println("HTTP begin failed");
    return;
  }

  int httpResponseCode = http.GET();
  if (httpResponseCode == HTTP_CODE_OK) {
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, http.getString());

    if (!error) {
      temperature = doc["main"]["temp"];
      wData[0] = doc["main"]["humidity"];
      wData[1] = doc["main"]["pressure"];
      wData[2] = doc["wind"]["speed"];

      int visibility = doc["visibility"];
      const char* description = doc["weather"][0]["description"];

      Wmsg = "#Description: " + String(description) + "  #Visbility: " + String(visibility)
           + " #Updated: " + rtc.getTime();

      Serial.print("Temperature: ");
      Serial.println(temperature);
      updateHistoryGraph(false);
    } else {
      Serial.print("JSON error: ");
      Serial.println(error.c_str());
    }
  } else {
    Serial.print("HTTP ERROR ");
    Serial.println(httpResponseCode);
    Wmsg = "API error " + String(httpResponseCode);
  }

  http.end();
}

void draw(bool force) {
  String clock = rtc.getTime().substring(0, 8);
  bool clockChanged = clock != lastClock;
  bool tickerChanged = Wmsg != lastTicker;

  if (!force && !clockChanged && !tickerChanged && (millis() - lastDrawMs < DRAW_INTERVAL_MS)) {
    return;
  }

  lastDrawMs = millis();
  lastClock = clock;
  lastTicker = Wmsg;
  errSprite.fillSprite(grays[10]);
  errSprite.setTextColor(grays[1], grays[10]);
  errSprite.drawString(Wmsg, ani, SY(4));

  sprite.fillSprite(TFT_BLACK);
  sprite.drawLine(SX(138), SY(10), SX(138), SY(164), grays[6]);
  sprite.drawLine(SX(100), SY(108), SX(134), SY(108), grays[6]);
  sprite.setTextDatum(0);

  // LEFTSIDE
  sprite.loadFont(midleFont);
  sprite.setTextColor(grays[1], TFT_BLACK);
  sprite.drawString("WEATHER", SX(6), SY(10));
  sprite.unloadFont();

  sprite.loadFont(font18);
  sprite.setTextColor(grays[7], TFT_BLACK);
  sprite.drawString("TOWN:", SX(6), SY(110));
  sprite.setTextColor(grays[2], TFT_BLACK);
  if (units == "metric")
    sprite.drawString("C", SX(14), SY(50));
  if (units == "imperial")
    sprite.drawString("F", SX(14), SY(50));

  sprite.setTextColor(grays[3], TFT_BLACK);
  sprite.drawString(town, SX(46), SY(110));
  sprite.fillCircle(SX(8), SY(52), SR(2), grays[2]);
  sprite.unloadFont();

  sprite.loadFont(tinyFont);
  sprite.setTextColor(grays[4], TFT_BLACK);
  sprite.drawString(rtc.getTime().substring(0, 5), SX(6), SY(132));
  sprite.unloadFont();

  sprite.setTextColor(grays[5], TFT_BLACK);
  sprite.drawString("INTERNET", SX(86), SY(10));
  sprite.drawString("STATION", SX(86), SY(20));
  sprite.setTextColor(grays[7], TFT_BLACK);
  sprite.drawString("SECONDS", SX(92), SY(157));

  sprite.setTextDatum(4);
  sprite.loadFont(bigFont);
  sprite.setTextColor(grays[0], TFT_BLACK);
  sprite.drawFloat(temperature, 1, SX(69), SY(80));
  sprite.unloadFont();

  sprite.fillRoundRect(SX(90), SY(132), SW(42), SH(22), SR(2), grays[2]);
  sprite.loadFont(font18);
  sprite.setTextColor(TFT_BLACK, grays[2]);
  sprite.drawString(rtc.getTime().substring(6, 8), SX(111), SY(144));
  sprite.unloadFont();

  sprite.setTextDatum(0);
  sprite.loadFont(font18);
  sprite.setTextColor(grays[1], TFT_BLACK);
  sprite.drawString("LAST 12 HOUR", SX(144), SY(10));
  sprite.unloadFont();

  sprite.fillRect(SX(144), SY(28), SW(84), SH(2), grays[10]);

  sprite.setTextColor(grays[3], TFT_BLACK);
  sprite.drawString("MIN:" + String(minT, 1), SX(254), SY(10));
  sprite.drawString("MAX:" + String(maxT, 1), SX(254), SY(20));
  sprite.fillSmoothRoundRect(SX(144), SY(34), SW(174), SH(60), SR(3), grays[10], bck);
  sprite.drawLine(SX(170), SY(39), SX(170), SY(88), TFT_WHITE);
  sprite.drawLine(SX(170), SY(88), SX(314), SY(88), TFT_WHITE);

  sprite.setTextDatum(4);

  for (int j = 0; j < 24; j++)
    for (int i = 0; i < PPgraph[j]; i++)
      sprite.fillRect(SX(173 + (j * 6)), SY(83 - (i * 4)), SW(4), SH(3), grays[2]);

  sprite.setTextColor(grays[2], grays[10]);
  sprite.drawString("MAX", SX(158), SY(42));
  sprite.drawString("MIN", SX(158), SY(86));

  sprite.loadFont(font18);
  sprite.setTextColor(grays[7], grays[10]);
  sprite.drawString("T", SX(158), SY(58));
  sprite.unloadFont();

  for (int i = 0; i < 3; i++) {
    sprite.fillSmoothRoundRect(SX(144 + (i * 60)), SY(100), SW(54), SH(32), SR(3), grays[9], bck);
    sprite.setTextColor(grays[3], grays[9]);
    sprite.drawString(PPlbl[i], SX(144 + (i * 60) + 27), SY(107));
    sprite.setTextColor(grays[2], grays[9]);
    sprite.loadFont(font18);
    sprite.drawString(String((int)wData[i]) + PPlblU[i], SX(144 + (i * 60) + 27), SY(124));
    sprite.unloadFont();
  }

  sprite.fillSmoothRoundRect(SX(144), SY(148), SW(174), SH(16), SR(2), grays[10], bck);
  errSprite.pushToSprite(&sprite, SX(148), SY(150));
  sprite.setTextColor(grays[4], bck);
  sprite.drawString("CURRENT WEATHER", SX(190), SY(141));
  sprite.setTextColor(grays[9], bck);
  sprite.drawString(String(counter), SX(310), SY(141));

  sprite.pushSprite(0, 0);
}

void updateData() {
  ani--;
  if (ani < SX(-420))
    ani = SX(100);

  if (millis() > timePased + 180000) {
    timePased = millis();
    counter++;
    getData();
    draw(true);

    if (counter >= 10) {
      setTime();
      counter = 0;
      updateHistoryGraph(true);
      draw(true);
    }
  }
}

void loop() {
  updateData();
  draw(false);
  delay(5);
}
