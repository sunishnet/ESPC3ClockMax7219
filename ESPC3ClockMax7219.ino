/*
 * ESP32-C3 Super Mini MAX7219 4-in-1 NTP Clock (Direct WiFi Version)
 *
 * Microcontroller: ESP32-C3 Super Mini
 * Display: MAX7219 4-in-1 Dot Matrix Display Module (32x8 LED matrix)
 * WiFi SSID: "4D"
 *
 * Hardware Wiring (ESP32-C3 Super Mini -> MAX7219):
 * - VCC  -> 5V (5V_IN pin)
 * - GND  -> GND
 * - DIN  -> GPIO 7
 * - CLK  -> GPIO 6
 * - CS   -> GPIO 5
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <MD_MAX72xx.h>
#include <MD_Parola.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_sntp.h>
#include <esp_wifi.h>
#include <time.h>

// Disable ESP32 brownout detector to prevent power-surge resets
#include "soc/rtc_cntl_reg.h"
#include "soc/soc.h"

#include "config.h"
#include "web_pages.h"

// Global Objects
MD_Parola display = MD_Parola(HARDWARE_TYPE, MAX7219_DIN_PIN, MAX7219_CLK_PIN,
                              MAX7219_CS_PIN, MAX_DEVICES);
WebServer server(80);
DNSServer dnsServer;
Preferences preferences;

// Clock Settings State
ClockSettings settings;

// System State Variables
bool isNtpSynced = false;
bool isWifiConnected = false;
bool isApMode = false;
bool isDisplayInitialized = false;
unsigned long lastDisplayUpdate = 0;
unsigned long lastDateScrollTime = 0;
bool colonVisible = true;

char currentTimeStr[16] = "00:00";
char currentDateStr[32] = "";

enum DisplayMode { MODE_CLOCK, MODE_SCROLL_DATE, MODE_MESSAGE, MODE_AP_CONFIG };
DisplayMode currentMode = MODE_CLOCK;

// Function Prototypes
void loadClockSettings();
void saveClockSettings();
void setupNtp();
void setupWebServer();
void updateTimeDisplay();
void handleStatusApi();
void handleGetSettingsApi();
void handlePostSettingsApi();
void handleLiveBrightnessApi();
void handleSyncApi();
void handleRestartApi();
void handleWifiScanApi();
void handleSaveWifiApi();
void handleResetWifiApi();
void startApMode();
void connectWiFi();
void setCompileTimeFallback();

// Callback when NTP time is synchronized
void ntpSyncCallback(struct timeval *tv) {
  isNtpSynced = true;
  Serial.println("\n[NTP] Time Synchronized Successfully!");
}

void setup() {
  // Disable brownout detector to prevent voltage-drop resets
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  Serial.setTxTimeoutMs(0); // Non-blocking Serial TX
  delay(500);

  Serial.println("\n==============================================");
  Serial.println("=== ESP32-C3 Super Mini MAX7219 NTP Clock ===");
  Serial.println("==============================================");

  // Step 1: Load Saved Settings
  Serial.println("[1] Loading Saved Settings...");
  loadClockSettings();

  // Set fallback RTC time automatically from compile date/time
  setCompileTimeFallback();

  // Step 2: Connect to WiFi
  connectWiFi();

  // Step 3: Setup NTP Time Sync
  sntp_set_time_sync_notification_cb(ntpSyncCallback);
  setupNtp();

  // Step 4: Initialize MAX7219 Display (after WiFi connects to prevent RF noise / power drop)
  Serial.println("[4] Initializing MAX7219 Display...");
  display.begin();
  display.setIntensity(settings.brightness);
  display.displayClear();
  display.setTextAlignment(PA_CENTER);
  isDisplayInitialized = true;

  // Step 5: Setup mDNS (http://clock.local)
  if (MDNS.begin(HOSTNAME)) {
    Serial.print("[mDNS] Responder started at http://");
    Serial.print(HOSTNAME);
    Serial.println(".local");
    MDNS.addService("http", "tcp", 80);
  }

  // Step 6: Start Web Server
  setupWebServer();
  server.begin();
  Serial.println("[Web] HTTP Server started on port 80");

  // Show connected IP on matrix briefly
  if (isWifiConnected) {
    String ipMsg = "IP: " + WiFi.localIP().toString();
    display.displayText(ipMsg.c_str(), PA_CENTER, 50, 1500, PA_SCROLL_LEFT,
                        PA_SCROLL_LEFT);
    while (!display.displayAnimate()) {
      yield();
    }
  }

  currentMode = MODE_CLOCK;
  display.setTextAlignment(PA_CENTER);
  lastDateScrollTime = millis();
}

void loop() {
  // Handle DNS requests in AP Captive Portal mode
  if (isApMode) {
    dnsServer.processNextRequest();
  }

  // Handle HTTP Web Server
  server.handleClient();

  // Handle Parola Matrix Display Animations
  if (isDisplayInitialized && display.displayAnimate()) {
    if (currentMode == MODE_SCROLL_DATE || currentMode == MODE_MESSAGE) {
      currentMode = MODE_CLOCK;
      display.setTextAlignment(PA_CENTER);
    } else if (currentMode == MODE_AP_CONFIG) {
      display.displayText("AP: 192.168.4.1", PA_CENTER, 60, 1000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
    }
  }

  // Monitor WiFi Connection status (ESP32 background auto-reconnect)
  if (!isApMode) {
    if (WiFi.status() == WL_CONNECTED) {
      if (!isWifiConnected) {
        isWifiConnected = true;
        Serial.println("\n[WiFi] Connected Successfully!");
        Serial.print("[WiFi] IP Address: ");
        Serial.println(WiFi.localIP().toString());

        // Show connected IP on matrix briefly
        if (isDisplayInitialized) {
          String ipMsg = "IP: " + WiFi.localIP().toString();
          display.displayText(ipMsg.c_str(), PA_CENTER, 50, 1500, PA_SCROLL_LEFT,
                              PA_SCROLL_LEFT);
        }
      }
    } else {
      if (isWifiConnected) {
        isWifiConnected = false;
        Serial.println("[WiFi] Connection lost. Retrying in background...");
      }
    }
  }

  // Update Time Display every 500ms
  unsigned long now = millis();
  if (isDisplayInitialized && now - lastDisplayUpdate >= 500) {
    lastDisplayUpdate = now;
    colonVisible = !colonVisible;

    if (currentMode == MODE_CLOCK) {
      updateTimeDisplay();
    }
  }

  // Periodically Scroll Date (if enabled)
  if (isDisplayInitialized && settings.showDateInterval && currentMode == MODE_CLOCK) {
    unsigned long intervalMs = (unsigned long)settings.dateIntervalMin * 60000;
    if (intervalMs > 0 && (now - lastDateScrollTime >= intervalMs)) {
      lastDateScrollTime = now;
      if (strlen(currentDateStr) > 0) {
        currentMode = MODE_SCROLL_DATE;
        display.displayText(currentDateStr, PA_CENTER, 60, 1000, PA_SCROLL_LEFT,
                            PA_SCROLL_LEFT);
      }
    }
  }

  yield();
}

// ====================================================================
// WiFi & AP Functions
// ====================================================================

void startApMode() {
  isApMode = true;
  isWifiConnected = false;
  currentMode = MODE_AP_CONFIG;

  Serial.println("\n==============================================");
  Serial.println("[AP] Starting Access Point Setup Portal...");
  Serial.println("==============================================");

  // Reset WiFi stack cleanly before configuring AP
  WiFi.disconnect(true);
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(200);

  // Configure WiFi as pure Access Point (WIFI_AP)
  WiFi.mode(WIFI_AP);
  delay(100);

  // Enable Channels 1 to 13
  wifi_country_t country = {"IN", 1, 13, 0, WIFI_COUNTRY_POLICY_AUTO};
  esp_wifi_set_country(&country);

  // Limit transmit power to 13 dBm (value 52) to avoid overloading the Super Mini LDO regulator
  esp_wifi_set_max_tx_power(52);

  bool success = WiFi.softAP(AP_SSID);
  if (success) {
    Serial.println("[AP] softAP started successfully!");
  } else {
    Serial.println("[AP] softAP failed to start!");
  }

  IPAddress apIP(192, 168, 4, 1);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

  dnsServer.start(53, "*", apIP);

  Serial.print("[AP] Access Point Name: ");
  Serial.println(AP_SSID);
  Serial.print("[AP] Setup Portal Address: http://");
  Serial.println(WiFi.softAPIP().toString());

  if (!isDisplayInitialized) {
    Serial.println("[AP] Initializing Display for setup portal UI...");
    display.begin();
    display.setIntensity(settings.brightness);
    display.displayClear();
    isDisplayInitialized = true;
  }
  display.displayText("AP: 192.168.4.1", PA_CENTER, 60, 1000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
}

void connectWiFi() {
  if (strlen(settings.wifiSsid) == 0) {
    Serial.println("[WiFi] No saved Wi-Fi SSID. Starting AP Setup Mode...");
    startApMode();
    return;
  }

  Serial.printf("[WiFi] Target SSID: '%s', Password: '%s'\n", settings.wifiSsid, settings.wifiPassword);
  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_8_5dBm); // Prevent weak LDO brownout on ESP32-C3 Super Mini

  // Begin standard connection (without WiFi.disconnect(true) which erases internally cached credentials)
  WiFi.begin(settings.wifiSsid, settings.wifiPassword);

  // Wait for connection (up to 20 seconds)
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    isWifiConnected = true;
    Serial.println("\n[WiFi] Connected Successfully!");
    Serial.print("[WiFi] IP Address: ");
    Serial.println(WiFi.localIP().toString());
  } else {
    isWifiConnected = false;
    Serial.println("\n[WiFi] Could not connect to saved Wi-Fi. Launching AP Setup Mode...");
    startApMode();
  }
}

// ====================================================================
// Helper & System Functions
// ====================================================================

void loadClockSettings() {
  preferences.begin("clock", false);
  String ssid = preferences.isKey("ssid") ? preferences.getString("ssid") : String(WIFI_SSID);
  String pass = preferences.isKey("pass") ? preferences.getString("pass") : String(WIFI_PASSWORD);
  String tz = preferences.isKey("tz") ? preferences.getString("tz") : String(DEFAULT_TIMEZONE);
  String ntp = preferences.isKey("ntp") ? preferences.getString("ntp") : String(DEFAULT_NTP_SERVER);
  settings.brightness = preferences.getUChar("bright", DEFAULT_BRIGHTNESS);
  settings.is24Hour = preferences.getBool("is24h", true);
  settings.showSeconds = preferences.getBool("sec", false);
  settings.showDateInterval = preferences.getBool("dateScrl", true);
  settings.dateIntervalMin = preferences.getUChar("dateInt", 1);
  preferences.end();

  strncpy(settings.wifiSsid, ssid.c_str(), sizeof(settings.wifiSsid));
  strncpy(settings.wifiPassword, pass.c_str(), sizeof(settings.wifiPassword));
  strncpy(settings.timezone, tz.c_str(), sizeof(settings.timezone));
  strncpy(settings.ntpServer, ntp.c_str(), sizeof(settings.ntpServer));

  // Fallback if password is empty
  if (strlen(settings.wifiPassword) == 0) {
    Serial.println("[Settings] Loaded password is empty! Falling back to WIFI_PASSWORD default.");
    strncpy(settings.wifiPassword, WIFI_PASSWORD, sizeof(settings.wifiPassword));
  }

  Serial.println("[Settings] Loaded preferences:");
  Serial.printf("  SSID: %s\n", settings.wifiSsid);
  Serial.printf("  Password: %s\n", settings.wifiPassword);
  Serial.printf("  Timezone: %s\n", settings.timezone);
  Serial.printf("  NTP Server: %s\n", settings.ntpServer);
  Serial.printf("  Brightness: %d\n", settings.brightness);
}

void saveClockSettings() {
  preferences.begin("clock", false);
  preferences.putString("ssid", settings.wifiSsid);
  preferences.putString("pass", settings.wifiPassword);
  preferences.putString("tz", settings.timezone);
  preferences.putString("ntp", settings.ntpServer);
  preferences.putUChar("bright", settings.brightness);
  preferences.putBool("is24h", settings.is24Hour);
  preferences.putBool("sec", settings.showSeconds);
  preferences.putBool("dateScrl", settings.showDateInterval);
  preferences.putUChar("dateInt", settings.dateIntervalMin);
  preferences.end();

  Serial.println("[Settings] Saved settings to Preferences.");
}

void setCompileTimeFallback() {
  const char* dateStr = __DATE__; // Format: "Mmm dd yyyy" e.g. "Jul 21 2026"
  const char* timeStr = __TIME__; // Format: "hh:mm:ss" e.g. "22:36:45"

  char monthStr[4] = {0};
  int day = 0, year = 0, hour = 0, min = 0, sec = 0;
  sscanf(dateStr, "%s %d %d", monthStr, &day, &year);
  sscanf(timeStr, "%d:%d:%d", &hour, &min, &sec);

  const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                          "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  int month = 0;
  for (int i = 0; i < 12; i++) {
    if (strncmp(monthStr, months[i], 3) == 0) {
      month = i;
      break;
    }
  }

  struct tm t = {0};
  t.tm_year = year - 1900;
  t.tm_mon  = month;
  t.tm_mday = day;
  t.tm_hour = hour;
  t.tm_min  = min;
  t.tm_sec  = sec;
  t.tm_isdst = -1;

  // Set TZ so mktime converts compile local time to UTC epoch correctly
  setenv("TZ", settings.timezone, 1);
  tzset();

  time_t epoch = mktime(&t);
  struct timeval tv = { .tv_sec = epoch, .tv_usec = 0 };
  settimeofday(&tv, NULL);

  Serial.printf("[RTC] Fallback time set to compile time: %02d:%02d:%02d (%02d/%02d/%04d)\n",
                hour, min, sec, day, month + 1, year);
}

void setupNtp() {
  Serial.printf("[NTP] Setting Timezone: %s | Server: %s\n", settings.timezone,
                settings.ntpServer);
  configTime(0, 0, settings.ntpServer, DEFAULT_NTP_SERVER_2);
  setenv("TZ", settings.timezone, 1);
  tzset();
}

void updateTimeDisplay() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    display.print("NO NTP");
    return;
  }

  int hour = timeinfo.tm_hour;
  if (!settings.is24Hour) {
    if (hour == 0)
      hour = 12;
    else if (hour > 12)
      hour -= 12;
  }

  char sep = colonVisible ? ':' : ' ';
  snprintf(currentTimeStr, sizeof(currentTimeStr), "%02d%c%02d", hour, sep,
           timeinfo.tm_min);

  strftime(currentDateStr, sizeof(currentDateStr), "%a %d %b %Y", &timeinfo);

  if (currentMode == MODE_CLOCK) {
    display.print(currentTimeStr);
  }
}

// ====================================================================
// Web Server Handlers & APIs
// ====================================================================

void setupWebServer() {
  server.on("/", HTTP_GET, []() {
    if (isApMode) {
      server.send_P(200, "text/html", HTML_WIFI_SETUP);
    } else {
      server.send_P(200, "text/html", HTML_INDEX);
    }
  });

  server.on("/api/status", HTTP_GET, handleStatusApi);
  server.on("/api/settings", HTTP_GET, handleGetSettingsApi);
  server.on("/api/settings", HTTP_POST, handlePostSettingsApi);
  server.on("/api/brightness", HTTP_POST, handleLiveBrightnessApi);
  server.on("/api/sync", HTTP_POST, handleSyncApi);
  server.on("/api/restart", HTTP_POST, handleRestartApi);
  server.on("/api/wifi_scan", HTTP_GET, handleWifiScanApi);
  server.on("/api/save_wifi", HTTP_POST, handleSaveWifiApi);
  server.on("/api/reset_wifi", HTTP_POST, handleResetWifiApi);

  server.onNotFound([]() {
    if (isApMode) {
      server.send_P(200, "text/html", HTML_WIFI_SETUP);
    } else {
      server.send(404, "text/plain", "404 Not Found");
    }
  });
}

void handleStatusApi() {
  struct tm timeinfo;
  char timeFull[16] = "--:--:--";
  if (getLocalTime(&timeinfo)) {
    snprintf(timeFull, sizeof(timeFull), "%02d:%02d:%02d", timeinfo.tm_hour,
             timeinfo.tm_min, timeinfo.tm_sec);
  }

  StaticJsonDocument<256> doc;
  doc["timeStr"] = timeFull;
  doc["dateStr"] = currentDateStr;
  doc["ip"] = WiFi.localIP().toString();
  doc["synced"] = isNtpSynced;

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handleGetSettingsApi() {
  StaticJsonDocument<256> doc;
  doc["brightness"] = settings.brightness;
  doc["is24Hour"] = settings.is24Hour;
  doc["showDateInterval"] = settings.showDateInterval;
  doc["timezone"] = settings.timezone;
  doc["ntpServer"] = settings.ntpServer;

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handlePostSettingsApi() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json",
                "{\"status\":\"error\",\"message\":\"Missing Body\"}");
    return;
  }

  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, server.arg("plain"));
  if (err) {
    server.send(400, "application/json",
                "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
    return;
  }

  if (doc.containsKey("brightness"))
    settings.brightness = doc["brightness"].as<uint8_t>();
  if (doc.containsKey("is24Hour"))
    settings.is24Hour = doc["is24Hour"].as<bool>();
  if (doc.containsKey("showDateInterval"))
    settings.showDateInterval = doc["showDateInterval"].as<bool>();
  if (doc.containsKey("timezone"))
    strncpy(settings.timezone, doc["timezone"], sizeof(settings.timezone));
  if (doc.containsKey("ntpServer"))
    strncpy(settings.ntpServer, doc["ntpServer"], sizeof(settings.ntpServer));

  saveClockSettings();

  display.setIntensity(settings.brightness);
  setupNtp();

  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleLiveBrightnessApi() {
  if (server.hasArg("val")) {
    uint8_t val = server.arg("val").toInt();
    if (val <= 15) {
      settings.brightness = val;
      display.setIntensity(val);
      server.send(200, "text/plain", "OK");
      return;
    }
  }
  server.send(400, "text/plain", "Invalid Value");
}

void handleSyncApi() {
  setupNtp();
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleRestartApi() {
  server.send(200, "application/json", "{\"status\":\"ok\"}");
  delay(500);
  ESP.restart();
}

void handleWifiScanApi() {
  int n = WiFi.scanNetworks();
  DynamicJsonDocument doc(2048);
  JsonArray arr = doc.to<JsonArray>();

  for (int i = 0; i < n; ++i) {
    JsonObject obj = arr.createNestedObject();
    obj["ssid"] = WiFi.SSID(i);
    obj["rssi"] = WiFi.RSSI(i);
    obj["channel"] = WiFi.channel(i);
  }

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handleSaveWifiApi() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing Body\"}");
    return;
  }

  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, server.arg("plain"));
  if (err) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
    return;
  }

  const char* ssid = doc["ssid"];
  const char* pass = doc["password"] | "";

  if (!ssid || strlen(ssid) == 0) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing SSID\"}");
    return;
  }

  strncpy(settings.wifiSsid, ssid, sizeof(settings.wifiSsid));
  strncpy(settings.wifiPassword, pass, sizeof(settings.wifiPassword));
  saveClockSettings();

  server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Saved. Rebooting...\"}");
  delay(1000);
  ESP.restart();
}

void handleResetWifiApi() {
  preferences.begin("clock", false);
  preferences.remove("ssid");
  preferences.remove("pass");
  preferences.end();

  server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"WiFi Reset. Rebooting...\"}");
  delay(1000);
  ESP.restart();
}
