#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <MD_MAX72xx.h>

// ====================================================================
// Hardcoded WiFi Credentials
// ====================================================================
#define WIFI_SSID "Nothing3"
#define WIFI_PASSWORD "9447010553"

// ====================================================================
// ESP32-C3 Super Mini Hardware Pin Definitions
// ====================================================================
// ESP32-C3 Super Mini Pinout Mapping to MAX7219 4-in-1 Dot Matrix Module:
//   MAX7219 VCC  -> ESP32-C3 5V (5V_IN pin)
//   MAX7219 GND  -> ESP32-C3 GND
//   MAX7219 DIN  -> GPIO 7
//   MAX7219 CLK  -> GPIO 6
//   MAX7219 CS   -> GPIO 5
// ====================================================================

#define MAX7219_DIN_PIN 4
#define MAX7219_CLK_PIN 3
#define MAX7219_CS_PIN 5

// ====================================================================
// MAX7219 Display Configuration
// ====================================================================
// Module type: FC16_HW is standard for most 4-in-1 green/red dot matrix
// modules.
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4

// Default Brightness: 0 (dimmest) to 15 (brightest)
#define DEFAULT_BRIGHTNESS 0

// ====================================================================
// Network & Web Portal Settings
// ====================================================================
#define HOSTNAME "clock"               // Accessible via http://clock.local
#define AP_SSID "ESP32-Clock-Setup"    // Wi-Fi Setup Access Point Name

// ====================================================================
// NTP & Timezone Defaults
// ====================================================================
#define DEFAULT_NTP_SERVER "pool.ntp.org"
#define DEFAULT_NTP_SERVER_2 "time.nist.gov"
#define DEFAULT_TIMEZONE "IST-5:30"

// Structure for runtime clock settings saved to ESP32 Preferences
struct ClockSettings {
  char wifiSsid[64];
  char wifiPassword[64];
  char timezone[64];
  char ntpServer[64];
  uint8_t brightness;      // 0 - 15
  bool is24Hour;           // true = 24h format, false = 12h format
  bool showSeconds;        // true = HH:MM:SS / false = HH:MM
  bool showDateInterval;   // true = scroll date every minute
  uint8_t dateIntervalMin; // Minutes between date scrolls
};

#endif // CONFIG_H
