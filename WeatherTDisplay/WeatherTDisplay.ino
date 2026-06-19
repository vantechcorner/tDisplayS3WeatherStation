#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <TFT_eSPI.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <ESP32Time.h>
#include <Preferences.h>

#define TFT_BL_PIN 4
#define ADC_POWER_PIN 14
#define BOOT_BTN 0

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);
ESP32Time rtc(0);
Preferences prefs;

int screenW;
int screenH;

//#################### EDIT THIS  ###################
int zone = 7;
const float cityLat = 16.0544f;
const float cityLon = 108.2022f;
String units = "metric";
// Leave empty and enter via WiFi setup portal, or paste your key here
String myAPI = "";
//#################### end of edits ###################

const char* ntpServer = "pool.ntp.org";
String server;

char apiKeyBuffer[48] = "";

int tickerX = 4;
unsigned long lastDraw = 0;
unsigned long lastFetch = 0;
unsigned long lastTicker = 0;
int counter = 0;
bool needApiReconfig = false;

unsigned short grays[13];

float temperature = 0;
float wData[3] = {0, 0, 0};
String Wmsg = "Loading weather...";

void initGrays();
void buildServer();
void loadApiKey();
void saveApiKey(const String& key);
bool apiKeyMissing();
void openConfigPortal();
void getData();

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

bool apiKeyMissing() {
  return myAPI.length() < 20;
}

void buildServer() {
  server = "https://api.openweathermap.org/data/2.5/weather?lat=" + String(cityLat, 4)
         + "&lon=" + String(cityLon, 4)
         + "&appid=" + myAPI + "&units=" + units;
}

void setTime() {
  configTime(3600 * zone, 0, ntpServer);
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    rtc.setTimeStruct(timeinfo);
  }
}

void setupBacklight() {
  pinMode(TFT_BL_PIN, OUTPUT);
  ledcSetup(0, 5000, 8);
  ledcAttachPin(TFT_BL_PIN, 0);
  ledcWrite(0, 220);
}

void openConfigPortal() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("WiFi/API setup", 40, screenH / 2 - 20, 2);
  tft.drawString("Connect: VolosWifiConf", 10, screenH / 2 + 4, 2);

  strncpy(apiKeyBuffer, myAPI.c_str(), sizeof(apiKeyBuffer) - 1);
  apiKeyBuffer[sizeof(apiKeyBuffer) - 1] = '\0';

  WiFiManager wifiManager;
  WiFiManagerParameter customApiKey(
    "apikey",
    "OpenWeather API Key",
    apiKeyBuffer,
    sizeof(apiKeyBuffer) - 1);
  wifiManager.addParameter(&customApiKey);
  wifiManager.setConfigPortalTimeout(0);

  if (!wifiManager.startConfigPortal("VolosWifiConf", "password")) {
    delay(2000);
    ESP.restart();
  }

  String newKey = customApiKey.getValue();
  if (newKey.length() >= 20) {
    saveApiKey(newKey);
    buildServer();
    Wmsg = "API key saved";
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(ADC_POWER_PIN, OUTPUT);
  digitalWrite(ADC_POWER_PIN, HIGH);
  pinMode(BOOT_BTN, INPUT_PULLUP);
  setupBacklight();

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  screenW = tft.width();
  screenH = tft.height();
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Connecting WiFi...", 24, screenH / 2 - 8, 2);

  sprite.createSprite(screenW, screenH);
  sprite.setTextDatum(TL_DATUM);

  loadApiKey();

  WiFiManager wifiManager;
  strncpy(apiKeyBuffer, myAPI.c_str(), sizeof(apiKeyBuffer) - 1);
  WiFiManagerParameter customApiKey(
    "apikey",
    "OpenWeather API Key",
    apiKeyBuffer,
    sizeof(apiKeyBuffer) - 1);
  wifiManager.addParameter(&customApiKey);
  wifiManager.setConfigPortalTimeout(apiKeyMissing() ? 0 : 30);

  if (!wifiManager.autoConnect("VolosWifiConf", "password")) {
    delay(3000);
    ESP.restart();
  }

  String newKey = customApiKey.getValue();
  if (newKey.length() >= 20) {
    saveApiKey(newKey);
  }

  buildServer();
  setTime();
  initGrays();
  getData();
}

void initGrays() {
  const uint8_t levels[] = {250, 220, 190, 160, 130, 100, 75, 55, 40, 28, 18, 12, 8};
  for (int i = 0; i < 13; i++) {
    uint8_t c = levels[i];
    grays[i] = tft.color565(c, c, c);
  }
}

void getData() {
  if (apiKeyMissing()) {
    Wmsg = "No API key - hold BOOT 3s to setup";
    needApiReconfig = true;
    return;
  }

  buildServer();

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  if (!http.begin(client, server)) {
    Wmsg = "HTTP connection failed";
    return;
  }

  int code = http.GET();
  if (code == HTTP_CODE_OK) {
    StaticJsonDocument<1024> doc;
    if (!deserializeJson(doc, http.getString())) {
      temperature = doc["main"]["temp"] | 0.0f;
      wData[0] = doc["main"]["humidity"] | 0.0f;
      wData[1] = doc["main"]["pressure"] | 0.0f;
      wData[2] = doc["wind"]["speed"] | 0.0f;
      const char* description = doc["weather"][0]["description"] | "n/a";
      int visibility = doc["visibility"] | 0;
      Wmsg = String(description) + "  |  Vis " + visibility + "m  |  " + rtc.getTime("%H:%M");
      Serial.printf("OK: %.1fC  hum=%.0f  press=%.0f  wind=%.1f\n",
                    temperature, wData[0], wData[1], wData[2]);
    }
  } else if (code == 401) {
    Serial.println("Invalid API key");
    Wmsg = "Invalid API key - hold BOOT 3s";
    needApiReconfig = true;
  } else {
    Serial.printf("HTTP error %d\n", code);
    Wmsg = "Weather API error " + String(code);
  }
  http.end();
}

void drawStatRow(int row, const char* label, const String& value) {
  int y = 22 + row * 30;
  sprite.fillRect(124, y, screenW - 128, 24, grays[9]);
  sprite.setTextColor(grays[5], grays[9]);
  sprite.drawString(label, 130, y + 4, 2);
  sprite.setTextDatum(TR_DATUM);
  sprite.setTextColor(grays[0], grays[9]);
  sprite.drawString(value, screenW - 6, y + 4, 2);
  sprite.setTextDatum(TL_DATUM);
}

void draw() {
  sprite.fillSprite(TFT_BLACK);

  sprite.fillRect(0, 0, screenW, 16, grays[11]);
  sprite.setTextColor(grays[0], grays[11]);
  sprite.drawString("WEATHER", 4, 3, 2);
  sprite.setTextDatum(TR_DATUM);
  sprite.setTextColor(grays[3], grays[11]);
  sprite.drawString(rtc.getTime("%H:%M"), screenW - 4, 3, 2);
  sprite.setTextDatum(TL_DATUM);

  sprite.setTextDatum(MC_DATUM);
  sprite.setTextColor(grays[0], TFT_BLACK);
  char tempBuf[12];
  snprintf(tempBuf, sizeof(tempBuf), "%.1f", temperature);
  sprite.drawString(tempBuf, 50, 48, 4);
  sprite.setTextColor(grays[2], TFT_BLACK);
  sprite.drawString(units == "metric" ? "C" : "F", 92, 48, 2);
  sprite.setTextDatum(TL_DATUM);
  sprite.setTextColor(grays[3], TFT_BLACK);
  sprite.drawString("Da Nang", 6, 82, 2);

  drawStatRow(0, "HUM", String((int)wData[0]) + "%");
  drawStatRow(1, "PRS", String((int)wData[1]));
  drawStatRow(2, "WND", String(wData[2], 1) + "m/s");

  sprite.fillRect(0, screenH - 15, screenW, 15, grays[10]);
  sprite.setTextColor(grays[1], grays[10]);
  sprite.drawString(Wmsg, tickerX, screenH - 13, 2);

  sprite.pushSprite(0, 0);
}

void loop() {
  unsigned long now = millis();

  // Hold BOOT button 3 seconds to re-enter API/WiFi setup
  static unsigned long bootPressStart = 0;
  if (digitalRead(BOOT_BTN) == LOW) {
    if (bootPressStart == 0) {
      bootPressStart = now;
    } else if (now - bootPressStart > 3000) {
      openConfigPortal();
      getData();
      bootPressStart = 0;
    }
  } else {
    bootPressStart = 0;
  }

  if (needApiReconfig && digitalRead(BOOT_BTN) == HIGH) {
    // Wait for user to hold BOOT; message already shown on screen
  }

  if (now - lastTicker > 80) {
    lastTicker = now;
    tickerX--;
    if (tickerX < -(int)Wmsg.length() * 6) {
      tickerX = screenW;
    }
  }

  if (now - lastFetch > 180000UL) {
    lastFetch = now;
    counter++;
    getData();
    if (counter >= 10) {
      setTime();
      counter = 0;
    }
  }

  if (now - lastDraw > 250) {
    lastDraw = now;
    draw();
  }
}
