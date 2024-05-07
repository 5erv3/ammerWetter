/****************************************************************************************************************************
  ConfigOnDoubleReset.ino
  For ESP8266 / ESP32 boards

  ESP_WiFiManager is a library for the ESP8266/ESP32 platform (https://github.com/esp8266/Arduino) to enable easy
  configuration and reconfiguration of WiFi credentials using a Captive Portal. Inspired by:
  http://www.esp8266.com/viewtopic.php?f=29&t=2520
  https://github.com/chriscook8/esp-arduino-apboot
  https://github.com/esp8266/Arduino/blob/master/libraries/DNSServer/examples/CaptivePortalAdvanced/

  Modified from Tzapu https://github.com/tzapu/WiFiManager
  and from Ken Taylor https://github.com/kentaylor

  Built by Khoi Hoang https://github.com/khoih-prog/ESP_WiFiManager
  Licensed under MIT license
 *****************************************************************************************************************************/
/****************************************************************************************************************************
   This example will open a configuration portal when the reset button is pressed twice.
   This method works well on Wemos boards which have a single reset button on board. It avoids using a pin for launching the configuration portal.

   How It Works
   1) ESP8266
   Save data in RTC memory
   2) ESP32
   Save data in EEPROM from address 256, size 512 bytes (both configurable)

   So when the device starts up it checks this region of ram for a flag to see if it has been recently reset.
   If so it launches a configuration portal, if not it sets the reset flag. After running for a while this flag is cleared so that
   it will only launch the configuration portal in response to closely spaced resets.

   Settings
   There are two values to be set in the sketch.

   DRD_TIMEOUT - Number of seconds to wait for the second reset. Set to 10 in the example.
   DRD_ADDRESS - The address in ESP8266 RTC RAM to store the flag. This memory must not be used for other purposes in the same sketch. Set to 0 in the example.

   This example, originally relied on the Double Reset Detector library from https://github.com/datacute/DoubleResetDetector
   To support ESP32, use ESP_DoubleResetDetector library from //https://github.com/khoih-prog/ESP_DoubleResetDetector
 *****************************************************************************************************************************/

#define TESTING 0

#include <sunset.h>
#define TIMEZONE    1
#define LATITUDE    48.17407
#define LONGITUDE   11.58409

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif
 
#if !( defined(ESP8266) ||  defined(ESP32) )
  #error This code is intended to run on the ESP8266 or ESP32 platform! Please check your Tools->Board setting.
#endif

#define ESP_WIFIMANAGER_VERSION_MIN_TARGET     "ESP_WiFiManager v1.7.3"

// Use from 0 to 4. Higher number, more debugging messages and memory usage.
#define _WIFIMGR_LOGLEVEL_    4

#include <FS.h>

//Ported to ESP32
#ifdef ESP32
  #include <esp_wifi.h>
  #include <WiFi.h>
  #include <WiFiClient.h>
  
  // From v1.1.0
  #include <WiFiMulti.h>
  WiFiMulti wifiMulti;

  // LittleFS has higher priority than SPIFFS
  #if ( ARDUINO_ESP32C3_DEV )
    // Currently, ESP32-C3 only supporting SPIFFS and EEPROM. Will fix to support LittleFS
    #define USE_LITTLEFS          false
    #define USE_SPIFFS            true
  #else
    #define USE_LITTLEFS    true
    #define USE_SPIFFS      false
  #endif

  #if USE_LITTLEFS
    // Use LittleFS
    #include "FS.h"

    // The library has been merged into esp32 core release 1.0.6
     #include <LITTLEFS.h>             // https://github.com/lorol/LITTLEFS
    
    FS* filesystem =      &LITTLEFS;
    #define FileFS        LITTLEFS
    #define FS_Name       "LittleFS"
  #elif USE_SPIFFS
    #include <SPIFFS.h>
    FS* filesystem =      &SPIFFS;
    #define FileFS        SPIFFS
    #define FS_Name       "SPIFFS"
  #else
    // Use FFat
    #include <FFat.h>
    FS* filesystem =      &FFat;
    #define FileFS        FFat
    #define FS_Name       "FFat"
  #endif
    //////
    
    #define ESP_getChipId()   ((uint32_t)ESP.getEfuseMac())

#define LED_BUILTIN       2
#define LED_ON            HIGH
#define LED_OFF           LOW

#else
  #include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
  //needed for library
  #include <DNSServer.h>
  #include <ESP8266WebServer.h>
  
  // From v1.1.0
  #include <ESP8266WiFiMulti.h>
  ESP8266WiFiMulti wifiMulti;
  
  #define USE_LITTLEFS      true
  
  #if USE_LITTLEFS
    #include <LittleFS.h>
    FS* filesystem =      &LittleFS;
    #define FileFS        LittleFS
    #define FS_Name       "LittleFS"
  #else
    FS* filesystem =      &SPIFFS;
    #define FileFS        SPIFFS
    #define FS_Name       "SPIFFS"
  #endif
  //////
  
  #define ESP_getChipId()   (ESP.getChipId())
  
  #define LED_ON      LOW
  #define LED_OFF     HIGH
#endif

// These defines must be put before #include <ESP_DoubleResetDetector.h>
// to select where to store DoubleResetDetector's variable.
// For ESP32, You must select one to be true (EEPROM or SPIFFS)
// For ESP8266, You must select one to be true (RTC, EEPROM, SPIFFS or LITTLEFS)
// Otherwise, library will use default EEPROM storage
#ifdef ESP32

  // These defines must be put before #include <ESP_DoubleResetDetector.h>
  // to select where to store DoubleResetDetector's variable.
  // For ESP32, You must select one to be true (EEPROM or SPIFFS)
  // Otherwise, library will use default EEPROM storage
  #if USE_LITTLEFS
    #define ESP_DRD_USE_LITTLEFS    true
    #define ESP_DRD_USE_SPIFFS      false
    #define ESP_DRD_USE_EEPROM      false
  #elif USE_SPIFFS
    #define ESP_DRD_USE_LITTLEFS    false
    #define ESP_DRD_USE_SPIFFS      true
    #define ESP_DRD_USE_EEPROM      false
  #else
    #define ESP_DRD_USE_LITTLEFS    false
    #define ESP_DRD_USE_SPIFFS      false
    #define ESP_DRD_USE_EEPROM      true
  #endif

#else //ESP8266

  // For DRD
  // These defines must be put before #include <ESP_DoubleResetDetector.h>
  // to select where to store DoubleResetDetector's variable.
  // For ESP8266, You must select one to be true (RTC, EEPROM, SPIFFS or LITTLEFS)
  // Otherwise, library will use default EEPROM storage
  #if USE_LITTLEFS
    #define ESP_DRD_USE_LITTLEFS    true
    #define ESP_DRD_USE_SPIFFS      false
  #else
    #define ESP_DRD_USE_LITTLEFS    false
    #define ESP_DRD_USE_SPIFFS      true
  #endif
  
  #define ESP_DRD_USE_EEPROM      false
  #define ESP8266_DRD_USE_RTC     false
#endif

#define DOUBLERESETDETECTOR_DEBUG       true  //false

#include <ESP_DoubleResetDetector.h>      //https://github.com/khoih-prog/ESP_DoubleResetDetector

// Number of seconds after reset during which a
// subseqent reset will be considered a double reset.
#define DRD_TIMEOUT 10

// RTC Memory Address for the DoubleResetDetector to use
#define DRD_ADDRESS 0

//DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);
DoubleResetDetector* drd;//////

// Onboard LED I/O pin on NodeMCU board
const int PIN_LED = 2; // D4 on NodeMCU and WeMos. GPIO2/ADC12 of ESP32. Controls the onboard LED.

// SSID and PW for Config Portal
String ssid       = "ESP_" + String(ESP_getChipId(), HEX);
String password;

// SSID and PW for your Router
String Router_SSID;
String Router_Pass;

// From v1.1.0
// You only need to format the filesystem once
//#define FORMAT_FILESYSTEM       true
#define FORMAT_FILESYSTEM         false

#define MIN_AP_PASSWORD_SIZE    8

#define SSID_MAX_LEN            32
//From v1.0.10, WPA2 passwords can be up to 63 characters long.
#define PASS_MAX_LEN            64

typedef struct
{
  char wifi_ssid[SSID_MAX_LEN];
  char wifi_pw  [PASS_MAX_LEN];
}  WiFi_Credentials;

typedef struct
{
  String wifi_ssid;
  String wifi_pw;
}  WiFi_Credentials_String;

#define NUM_WIFI_CREDENTIALS      2

// Assuming max 491 chars
#define TZNAME_MAX_LEN            50
#define TIMEZONE_MAX_LEN          50

typedef struct
{
  WiFi_Credentials  WiFi_Creds [NUM_WIFI_CREDENTIALS];
  char TZ_Name[TZNAME_MAX_LEN];     // "America/Toronto"
  char TZ[TIMEZONE_MAX_LEN];        // "EST5EDT,M3.2.0,M11.1.0"
  uint16_t checksum;
} WM_Config;

WM_Config         WM_config;

#define  CONFIG_FILENAME              F("/wifi_cred.dat")
//////

// Indicates whether ESP has WiFi credentials saved from previous session, or double reset detected
bool initialConfig = false;

// Use false if you don't like to display Available Pages in Information Page of Config Portal
// Comment out or use true to display Available Pages in Information Page of Config Portal
// Must be placed before #include <ESP_WiFiManager.h>
#define USE_AVAILABLE_PAGES     true    //false

// From v1.0.10 to permit disable/enable StaticIP configuration in Config Portal from sketch. Valid only if DHCP is used.
// You'll loose the feature of dynamically changing from DHCP to static IP, or vice versa
// You have to explicitly specify false to disable the feature.
//#define USE_STATIC_IP_CONFIG_IN_CP          false

// Use false to disable NTP config. Advisable when using Cellphone, Tablet to access Config Portal.
// See Issue 23: On Android phone ConfigPortal is unresponsive (https://github.com/khoih-prog/ESP_WiFiManager/issues/23)
#define USE_ESP_WIFIMANAGER_NTP     true

// Just use enough to save memory. On ESP8266, can cause blank ConfigPortal screen
// if using too much memory
#define USING_AFRICA        false
#define USING_AMERICA       false
#define USING_ANTARCTICA    false
#define USING_ASIA          false
#define USING_ATLANTIC      false
#define USING_AUSTRALIA     false
#define USING_EUROPE        true
#define USING_INDIAN        false
#define USING_PACIFIC       false
#define USING_ETC_GMT       false

// Use true to enable CloudFlare NTP service. System can hang if you don't have Internet access while accessing CloudFlare
// See Issue #21: CloudFlare link in the default portal (https://github.com/khoih-prog/ESP_WiFiManager/issues/21)
#define USE_CLOUDFLARE_NTP          false

// New in v1.0.11
#define USING_CORS_FEATURE          true
//////

// Use USE_DHCP_IP == true for dynamic DHCP IP, false to use static IP which you have to change accordingly to your network
#if (defined(USE_STATIC_IP_CONFIG_IN_CP) && !USE_STATIC_IP_CONFIG_IN_CP)
// Force DHCP to be true
#if defined(USE_DHCP_IP)
#undef USE_DHCP_IP
#endif
#define USE_DHCP_IP     true
#else
// You can select DHCP or Static IP here
#define USE_DHCP_IP     true
//#define USE_DHCP_IP     false
#endif

#if ( USE_DHCP_IP )
// Use DHCP
#warning Using DHCP IP
IPAddress stationIP   = IPAddress(0, 0, 0, 0);
IPAddress gatewayIP   = IPAddress(192, 168, 2, 1);
IPAddress netMask     = IPAddress(255, 255, 255, 0);
#else
// Use static IP
#warning Using static IP
#ifdef ESP32
IPAddress stationIP   = IPAddress(192, 168, 2, 232);
#else
IPAddress stationIP   = IPAddress(192, 168, 2, 186);
#endif

IPAddress gatewayIP   = IPAddress(192, 168, 2, 1);
IPAddress netMask     = IPAddress(255, 255, 255, 0);
#endif

#define USE_CONFIGURABLE_DNS      true

IPAddress dns1IP      = gatewayIP;
IPAddress dns2IP      = IPAddress(8, 8, 8, 8);

#define USE_CUSTOM_AP_IP          false

// New in v1.4.0
IPAddress APStaticIP  = IPAddress(192, 168, 232, 1);
IPAddress APStaticGW  = IPAddress(192, 168, 232, 1);
IPAddress APStaticSN  = IPAddress(255, 255, 255, 0);

#include <ESP_WiFiManager.h>              //https://github.com/khoih-prog/ESP_WiFiManager

// Function Prototypes
uint8_t connectMultiWiFi();

#include <FastLED.h>

#define LED_PIN     13
#define NUM_LEDS    50
#define BRIGHTNESS  255
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

#define UPDATES_PER_SECOND 100

CRGBPalette16 currentPalette;
TBlendType    currentBlending;

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;

typedef enum led_states_e {
  LED_STATE_NONE = 0,
  LED_STATE_INIT = 1,
  LED_STATE_INIT_ERR,
  LED_STATE_CONFIG,
  LED_STATE_NOCONNECTION,
  LED_STATE_WIFI_OK_TIME_WAITING,
  LED_STATE_NORMAL,
  LED_STATE_OFF
}led_state;

led_state led_statemachine_status = LED_STATE_INIT;

SunSet sun;

typedef enum sunstate_e {
  SUNSTATE_SUN_UP = 0,
  SUNSTATE_SUN_DOWN_BEFORE_MIDNIGHT,
  SUNSTATE_SUN_DOWN_AFTER_MIDNIGHT
}sunstate;

///////////////////////////////////////////
// New in v1.4.0
/******************************************
 * // Defined in ESP_WiFiManager.h
typedef struct
{
  IPAddress _ap_static_ip;
  IPAddress _ap_static_gw;
  IPAddress _ap_static_sn;

}  WiFi_AP_IPConfig;

typedef struct
{
  IPAddress _sta_static_ip;
  IPAddress _sta_static_gw;
  IPAddress _sta_static_sn;
#if USE_CONFIGURABLE_DNS  
  IPAddress _sta_static_dns1;
  IPAddress _sta_static_dns2;
#endif
}  WiFi_STA_IPConfig;
******************************************/

WiFi_AP_IPConfig  WM_AP_IPconfig;
WiFi_STA_IPConfig WM_STA_IPconfig;

void initAPIPConfigStruct(WiFi_AP_IPConfig &in_WM_AP_IPconfig)
{
  in_WM_AP_IPconfig._ap_static_ip   = APStaticIP;
  in_WM_AP_IPconfig._ap_static_gw   = APStaticGW;
  in_WM_AP_IPconfig._ap_static_sn   = APStaticSN;
}

void initSTAIPConfigStruct(WiFi_STA_IPConfig &in_WM_STA_IPconfig)
{
  in_WM_STA_IPconfig._sta_static_ip   = stationIP;
  in_WM_STA_IPconfig._sta_static_gw   = gatewayIP;
  in_WM_STA_IPconfig._sta_static_sn   = netMask;
#if USE_CONFIGURABLE_DNS  
  in_WM_STA_IPconfig._sta_static_dns1 = dns1IP;
  in_WM_STA_IPconfig._sta_static_dns2 = dns2IP;
#endif
}

void displayIPConfigStruct(WiFi_STA_IPConfig in_WM_STA_IPconfig)
{
  LOGERROR3(F("stationIP ="), in_WM_STA_IPconfig._sta_static_ip, ", gatewayIP =", in_WM_STA_IPconfig._sta_static_gw);
  LOGERROR1(F("netMask ="), in_WM_STA_IPconfig._sta_static_sn);
#if USE_CONFIGURABLE_DNS
  LOGERROR3(F("dns1IP ="), in_WM_STA_IPconfig._sta_static_dns1, ", dns2IP =", in_WM_STA_IPconfig._sta_static_dns2);
#endif
}

void configWiFi(WiFi_STA_IPConfig in_WM_STA_IPconfig)
{
  #if USE_CONFIGURABLE_DNS  
    // Set static IP, Gateway, Subnetmask, DNS1 and DNS2. New in v1.0.5
    WiFi.config(in_WM_STA_IPconfig._sta_static_ip, in_WM_STA_IPconfig._sta_static_gw, in_WM_STA_IPconfig._sta_static_sn, in_WM_STA_IPconfig._sta_static_dns1, in_WM_STA_IPconfig._sta_static_dns2);  
  #else
    // Set static IP, Gateway, Subnetmask, Use auto DNS1 and DNS2.
    WiFi.config(in_WM_STA_IPconfig._sta_static_ip, in_WM_STA_IPconfig._sta_static_gw, in_WM_STA_IPconfig._sta_static_sn);
  #endif 
}

///////////////////////////////////////////

uint8_t connectMultiWiFi()
{
  static int connectioncounter = 0;
#if ESP32
  // For ESP32, this better be 0 to shorten the connect time.
  // For ESP32-S2/C3, must be > 500
  #if ( USING_ESP32_S2 || USING_ESP32_C3 )
    #define WIFI_MULTI_1ST_CONNECT_WAITING_MS           500L
  #else
    // For ESP32 core v1.0.6, must be >= 500
    #define WIFI_MULTI_1ST_CONNECT_WAITING_MS           800L
  #endif
#else
  // For ESP8266, this better be 2200 to enable connect the 1st time
  #define WIFI_MULTI_1ST_CONNECT_WAITING_MS             2200L
#endif

#define WIFI_MULTI_CONNECT_WAITING_MS                   500L

  uint8_t status;

  //WiFi.mode(WIFI_STA);

  LOGERROR(F("ConnectMultiWiFi with :"));

  if ( (Router_SSID != "") && (Router_Pass != "") )
  {
    LOGERROR3(F("* Flash-stored Router_SSID = "), Router_SSID, F(", Router_Pass = "), Router_Pass );
    LOGERROR3(F("* Add SSID = "), Router_SSID, F(", PW = "), Router_Pass );
    wifiMulti.addAP(Router_SSID.c_str(), Router_Pass.c_str());
  }

  for (uint8_t i = 0; i < NUM_WIFI_CREDENTIALS; i++)
  {
    // Don't permit NULL SSID and password len < MIN_AP_PASSWORD_SIZE (8)
    if ( (String(WM_config.WiFi_Creds[i].wifi_ssid) != "") && (strlen(WM_config.WiFi_Creds[i].wifi_pw) >= MIN_AP_PASSWORD_SIZE) )
    {
      LOGERROR3(F("* Additional SSID = "), WM_config.WiFi_Creds[i].wifi_ssid, F(", PW = "), WM_config.WiFi_Creds[i].wifi_pw );
    }
  }

  LOGERROR(F("Connecting MultiWifi..."));

  //WiFi.mode(WIFI_STA);

#if !USE_DHCP_IP
  // New in v1.4.0
  configWiFi(WM_STA_IPconfig);
  //////
#endif

  int i = 0;
  
  status = wifiMulti.run();
  delay(WIFI_MULTI_1ST_CONNECT_WAITING_MS);

  while ( ( i++ < 20 ) && ( status != WL_CONNECTED ) )
  {
    status = WiFi.status();

    if ( status == WL_CONNECTED )
      break;
    else
      delay(WIFI_MULTI_CONNECT_WAITING_MS);
  }

  if ( status == WL_CONNECTED )
  {
    LOGERROR1(F("WiFi connected after time: "), i);
    LOGERROR3(F("SSID:"), WiFi.SSID(), F(",RSSI="), WiFi.RSSI());
    LOGERROR3(F("Channel:"), WiFi.channel(), F(",IP address:"), WiFi.localIP() );
    connectioncounter = 0;
  }
  else
  {
    connectioncounter++;
    LOGERROR(F("WiFi not connected"));

    // To avoid unnecessary DRD
    drd->loop();

    if (connectioncounter > 20){
      Serial.println("No connection for a long time, restarting ESP in 2s & switching off LED.");
      led_statemachine_status = LED_STATE_OFF;
      delay(2000);
#if ESP8266      
      ESP.reset();
#else
      ESP.restart();
#endif 
    } 
  }

  return status;
}

#if USE_ESP_WIFIMANAGER_NTP

void printLocalTime()
{
#if ESP8266
  static time_t now;
  
  now = time(nullptr);
  
  if ( now > 1451602800 )
  {
    Serial.print("Local Date/Time: ");
    Serial.print(ctime(&now));
  }
#else
  struct tm timeinfo;

  getLocalTime( &timeinfo );

  // Valid only if year > 2000. 
  // You can get from timeinfo : tm_year, tm_mon, tm_mday, tm_hour, tm_min, tm_sec
  if (timeinfo.tm_year > 100 )
  {
    Serial.print("Local Date/Time: ");
    Serial.print( asctime( &timeinfo ) );
  }
#endif
}

#endif

void heartBeatPrint()
{
#if USE_ESP_WIFIMANAGER_NTP
  printLocalTime();
#else
  static int num = 1;

  if (WiFi.status() == WL_CONNECTED)
    Serial.print(F("H"));        // H means connected to WiFi
  else
    Serial.print(F("F"));        // F means not connected to WiFi

  if (num == 80)
  {
    Serial.println();
    num = 1;
  }
  else if (num++ % 10 == 0)
  {
    Serial.print(F(" "));
  }
#endif  
}

void check_WiFi()
{
  uint8_t status;
  if ( (WiFi.status() != WL_CONNECTED) )
  {
    led_statemachine_status = LED_STATE_NOCONNECTION;
    Serial.println(F("\nWiFi lost. Call connectMultiWiFi in loop"));
    status = connectMultiWiFi();
  }
  if (status == WL_CONNECTED){
    led_statemachine_status = LED_STATE_NORMAL;
  }
}

void check_status()
{
  static ulong checkstatus_timeout  = 0;
  static ulong checkwifi_timeout    = 0;

  static ulong current_millis;

#define WIFICHECK_INTERVAL    1000L

#if USE_ESP_WIFIMANAGER_NTP
  #define HEARTBEAT_INTERVAL    60000L
#else
  #define HEARTBEAT_INTERVAL    10000L
#endif

  current_millis = millis();

  // Check WiFi every WIFICHECK_INTERVAL (1) seconds.
  if ((current_millis > checkwifi_timeout) || (checkwifi_timeout == 0))
  {
    check_WiFi();
    checkwifi_timeout = current_millis + WIFICHECK_INTERVAL;
  }

  // Print hearbeat every HEARTBEAT_INTERVAL (10) seconds.
  if ((current_millis > checkstatus_timeout) || (checkstatus_timeout == 0))
  {
    heartBeatPrint();
    checkstatus_timeout = current_millis + HEARTBEAT_INTERVAL;
  }
}

int calcChecksum(uint8_t* address, uint16_t sizeToCalc)
{
  uint16_t checkSum = 0;
  
  for (uint16_t index = 0; index < sizeToCalc; index++)
  {
    checkSum += * ( ( (byte*) address ) + index);
  }

  return checkSum;
}

bool loadConfigData()
{
  File file = FileFS.open(CONFIG_FILENAME, "r");
  LOGERROR(F("LoadWiFiCfgFile "));

  memset((void*) &WM_config,       0, sizeof(WM_config));

  // New in v1.4.0
  memset((void*) &WM_STA_IPconfig, 0, sizeof(WM_STA_IPconfig));
  //////

  if (file)
  {
    file.readBytes((char *) &WM_config,   sizeof(WM_config));

    // New in v1.4.0
    file.readBytes((char *) &WM_STA_IPconfig, sizeof(WM_STA_IPconfig));
    //////

    file.close();
    LOGERROR(F("OK"));

    if ( WM_config.checksum != calcChecksum( (uint8_t*) &WM_config, sizeof(WM_config) - sizeof(WM_config.checksum) ) )
    {
      LOGERROR(F("WM_config checksum wrong"));
      
      return false;
    }
    
    // New in v1.4.0
    displayIPConfigStruct(WM_STA_IPconfig);
    //////

    return true;
  }
  else
  {
    LOGERROR(F("failed"));

    return false;
  }
}

void saveConfigData()
{
  File file = FileFS.open(CONFIG_FILENAME, "w");
  LOGERROR(F("SaveWiFiCfgFile "));

  if (file)
  {
    WM_config.checksum = calcChecksum( (uint8_t*) &WM_config, sizeof(WM_config) - sizeof(WM_config.checksum) );
    
    file.write((uint8_t*) &WM_config, sizeof(WM_config));

    displayIPConfigStruct(WM_STA_IPconfig);

    // New in v1.4.0
    file.write((uint8_t*) &WM_STA_IPconfig, sizeof(WM_STA_IPconfig));
    //////

    file.close();
    LOGERROR(F("OK"));
  }
  else
  {
    LOGERROR(F("failed"));
  }
}

void TaskWifiHandler( void *pvParameters );
void TaskLedHandler( void *pvParameters );

void setup()
{
  // initialize serial communication at 115200 bits per second:
  Serial.begin(115200);
  while (!Serial);

  pinMode(PIN_LED, OUTPUT);
  sun.setPosition(LATITUDE, LONGITUDE, TIMEZONE);
  
  // Now set up two tasks to run independently.
  xTaskCreatePinnedToCore(
    TaskWifiHandler
    ,  "TaskWifi"   // A name just for humans
    ,  1024 * 12  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL 
    ,  ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(
    TaskLedHandler
    ,  "TaskLED"
    ,  1024 * 12  // Stack size
    ,  NULL
    ,  1  // Priority
    ,  NULL 
    ,  ARDUINO_RUNNING_CORE);

  // Now the task scheduler, which takes over control of scheduling individual tasks, is automatically started.
}

sunstate getSunState(int hour, int min, int year, int month, int day, int daylightsaving){
  int minpastmidnight = hour * 60 + min;

  sun.setCurrentDate(year+1900, month+1, day);
  sun.setTZOffset(TIMEZONE+daylightsaving);
  double sunrise = sun.calcSunrise();
  double sunset = sun.calcSunset();

  int sunset_in = sunset - minpastmidnight;
  int sunrise_in = sunrise - minpastmidnight;

  Serial.print(F("sunrise in "));
  Serial.print(sunrise_in);
  Serial.print(F(" min, sunset in "));
  Serial.print(sunset_in);
  Serial.print(F(" ,minpastmidnight: "));
  Serial.println(minpastmidnight);   

  if (sunset_in > 0 && sunrise_in < 0){
    return SUNSTATE_SUN_UP;
  } 

  if (sunrise_in > 0){
    return SUNSTATE_SUN_DOWN_AFTER_MIDNIGHT;
  }

  return SUNSTATE_SUN_DOWN_BEFORE_MIDNIGHT;
}

void setSingleLED(int8_t h, int8_t m, CRGB color, CHSV background_color){
  static int lastLedNb = 0;
  static CRGB lastcolor = CRGB::Black;
  
  if (h >= 12){
    h -= 12;
  }

  float hour_f = h;
  float min_f = m;
  float led_nb = NUM_LEDS;

  // calc leds per hour, use 13 because we have space before the 1 and after 12
  float ledsperhour = led_nb / 12.0;
  float lednb_f = (hour_f * ledsperhour) +  ((min_f * ledsperhour) / 60.0) + 1.0;
  int lednb = (int) lednb_f;

  if (lastLedNb != lednb || lastcolor != color){
    fill_solid( &(leds[0]), NUM_LEDS, background_color );
    
    lastLedNb = lednb;
    lastcolor = color;
    if (NUM_LEDS - lednb > 0){
      leds[NUM_LEDS - lednb -1] = color;
    } 
    leds[NUM_LEDS - lednb] = color;
    if (NUM_LEDS - lednb < NUM_LEDS) {
      leds[NUM_LEDS - lednb +1] = color;
    }

    FastLED.show();
    
    Serial.print(F("hour/min= "));
    Serial.print(h);
    Serial.print(m);
    Serial.print(F(" single led nb_f = , "));
    Serial.print(lednb_f);
    Serial.print(F(" single led nb = "));
    Serial.println(lednb);    
  }  
}

void updateLedTime(int8_t test_hour=-1, int8_t test_min=-1) {

  CHSV daylight_color_back = CHSV( 0, 0, 0);
  CHSV night_color_back = CHSV( HUE_PURPLE, 80, 20);
  CHSV deep_night_color_back = CHSV( 0, 0, 0);
  CRGB color_sun = CRGB::Yellow;
  CRGB color_moon = CRGB::Orange;
  CRGB color_deep_moon = CRGB(10,53,40);

  CHSV background_color;
  CRGB indicator_color;

  sunstate sun_state;


  struct tm timeinfo;
  getLocalTime( &timeinfo );

  if (timeinfo.tm_year > 100 )
  {
#if TESTING
    sun_state = getSunState(test_hour, test_min, 2021, 11, 21, 0);
#else
    sun_state = getSunState(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_year, timeinfo.tm_mon, timeinfo.tm_mday, timeinfo.tm_isdst);
#endif
    switch (sun_state) {
      case SUNSTATE_SUN_UP:
        background_color = daylight_color_back;
        indicator_color = color_sun;
      break;
      case SUNSTATE_SUN_DOWN_BEFORE_MIDNIGHT:
        background_color = night_color_back;
        indicator_color = color_moon;
      break;
      case SUNSTATE_SUN_DOWN_AFTER_MIDNIGHT:
        background_color = deep_night_color_back;
        indicator_color = color_deep_moon;
      break;
    }

#if TESTING
    setSingleLED(test_hour, test_min, indicator_color, background_color);
#else
    setSingleLED(timeinfo.tm_hour, timeinfo.tm_min, indicator_color, background_color);
#endif
  } else {
    Serial.print(F("ERROR: Time not yet set"));
    led_statemachine_status = LED_STATE_WIFI_OK_TIME_WAITING;
  }
}

ulong last_ledupdate = 0;

void loop()
{
    // Empty. Things are done in Tasks.
}

void FillLEDsFromPaletteColors( uint8_t colorIndex)
{
    uint8_t brightness = 255;
    
    for( int i = 0; i < NUM_LEDS; ++i) {
        leds[i] = ColorFromPalette( currentPalette, colorIndex, brightness, currentBlending);
        colorIndex += 3;
    }
}


// There are several different palettes of colors demonstrated here.
//
// FastLED provides several 'preset' palettes: RainbowColors_p, RainbowStripeColors_p,
// OceanColors_p, CloudColors_p, LavaColors_p, ForestColors_p, and PartyColors_p.
//
// Additionally, you can manually define your own color palettes, or you can write
// code that creates color palettes on the fly.  All are shown here.

bool ChangePalettePeriodically()
{
    static ulong starttime = millis(); 
    uint8_t secondHand = ((millis() - starttime) / 1000) % 60;
    static uint8_t lastSecond = 99;
    
    if( lastSecond != secondHand) {
        lastSecond = secondHand;
        if( secondHand ==  0)  { currentPalette = RainbowColors_p;         currentBlending = LINEARBLEND; }
        if( secondHand == 10)  { currentPalette = RainbowStripeColors_p;   currentBlending = LINEARBLEND; }
        if( secondHand == 20)  { SetupTotallyRandomPalette();              currentBlending = LINEARBLEND; }
        if( secondHand == 25)  { SetupBlackAndWhiteStripedPalette();       currentBlending = LINEARBLEND; }
        if( secondHand == 30)  { currentPalette = CloudColors_p;           currentBlending = LINEARBLEND; }
        if( secondHand == 35)  { currentPalette = PartyColors_p;           currentBlending = LINEARBLEND; }
        if( secondHand == 40)  { currentPalette = myRedWhiteBluePalette_p; currentBlending = LINEARBLEND; return false;}
    }
    return true;
}

// This function fills the palette with totally random colors.
void SetupTotallyRandomPalette()
{
    for( int i = 0; i < 16; ++i) {
        currentPalette[i] = CHSV( random8(), 255, random8());
    }
}

// This function sets up a palette of black and white stripes,
// using code.  Since the palette is effectively an array of
// sixteen CRGB colors, the various fill_* functions can be used
// to set them up.
void SetupBlackAndWhiteStripedPalette()
{
    // 'black out' all 16 palette entries...
    fill_solid( currentPalette, 16, CRGB::Black);
    // and set every fourth one to white.
    currentPalette[0] = CRGB::White;
    currentPalette[4] = CRGB::White;
    currentPalette[8] = CRGB::White;
    currentPalette[12] = CRGB::White;
    
}

// This function sets up a palette of purple and green stripes.
void SetupPurpleAndGreenPalette()
{
    CRGB purple = CHSV( HUE_PURPLE, 255, 255);
    CRGB green  = CHSV( HUE_GREEN, 255, 255);
    CRGB black  = CRGB::Black;
    
    currentPalette = CRGBPalette16(
                                   green,  green,  black,  black,
                                   purple, purple, black,  black,
                                   green,  green,  black,  black,
                                   purple, purple, black,  black );
}


// This example shows how to set up a static color palette
// which is stored in PROGMEM (flash), which is almost always more
// plentiful than RAM.  A static PROGMEM palette like this
// takes up 64 bytes of flash.
const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM =
{
    CRGB::Red,
    CRGB::Gray, // 'white' is too bright compared to red and blue
    CRGB::Blue,
    CRGB::Black,
    
    CRGB::Red,
    CRGB::Gray,
    CRGB::Blue,
    CRGB::Black,
    
    CRGB::Red,
    CRGB::Red,
    CRGB::Gray,
    CRGB::Gray,
    CRGB::Blue,
    CRGB::Blue,
    CRGB::Black,
    CRGB::Black
};

void TaskLedHandler(void *pvParameters)
{  
  static led_state last_state = LED_STATE_NONE;
  static led_state last_state_report = LED_STATE_NONE;
  static uint8_t startIndex = 0;
  
  (void) pvParameters;
  Serial.println("LED Task Started");
  
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );  
  FastLED.setBrightness(  BRIGHTNESS / 3 );
  fill_solid( &(leds[0]), NUM_LEDS, CRGB::Black );
  FastLED.show();

  delay(1000);

  for (;;) {
    if (last_state_report != led_statemachine_status){      
      Serial.print("LED state change from ");
      Serial.print(last_state_report);
      Serial.print(" to ");
      Serial.println(led_statemachine_status);
      last_state_report = led_statemachine_status;
      startIndex = 0;
    }
    
    switch(led_statemachine_status){
      case LED_STATE_INIT: 
        FastLED.setBrightness(BRIGHTNESS/3);
        currentPalette = RainbowColors_p;
        currentBlending = LINEARBLEND;        
        startIndex = startIndex + 1;
        FillLEDsFromPaletteColors(startIndex);
        FastLED.show();
        FastLED.delay(1000 / UPDATES_PER_SECOND);
        break;
        
      case LED_STATE_INIT_ERR:
      case LED_STATE_NOCONNECTION:
        FastLED.setBrightness(BRIGHTNESS/3);
        currentPalette = LavaColors_p;
        currentBlending = LINEARBLEND;        
        startIndex = startIndex + 1;
        FillLEDsFromPaletteColors(startIndex);
        FastLED.show();
        FastLED.delay(1000 / UPDATES_PER_SECOND);
      break;
      
      case LED_STATE_CONFIG:
        FastLED.setBrightness(BRIGHTNESS/3);
        currentPalette = CloudColors_p;
        currentBlending = LINEARBLEND;        
        startIndex = startIndex + 1;
        FillLEDsFromPaletteColors(startIndex);
        FastLED.show();
        FastLED.delay(1000 / UPDATES_PER_SECOND);
      break;
        
      case LED_STATE_WIFI_OK_TIME_WAITING:
        FastLED.setBrightness(BRIGHTNESS/3);
        fill_solid( &(leds[0]), NUM_LEDS, CRGB::Green);
        FastLED.show();
        
        struct tm timeinfo;
        getLocalTime( &timeinfo );      
        if (timeinfo.tm_year > 100 )
        {
           led_statemachine_status = LED_STATE_NORMAL;
           last_ledupdate = 0;
        }        
      break;        
        
      case LED_STATE_NORMAL:
        if (last_state != LED_STATE_NORMAL){
          last_state = LED_STATE_NORMAL;
          FastLED.setBrightness(BRIGHTNESS);
          currentPalette = RainbowColors_p;
          currentBlending = LINEARBLEND;
        } 
        FastLED.setBrightness(BRIGHTNESS); 
        #if TESTING
        for (int i=0; i< 24; i++){
          for (int j=0; j<60; j+=5){
            updateLedTime(i,j);
            delay(50);
          }
        }
        #else
        if (millis() - last_ledupdate > 5 * 1000 ) {
          last_ledupdate = millis();
          updateLedTime();
        }
        #endif  
      break;
        
      case LED_STATE_OFF:
        fill_solid( &(leds[0]), NUM_LEDS, CRGB::Black);
        FastLED.show();
        delay(100);
      break;
        
      default: 
        delay(100);
      break;
    }
  }  
}


void TaskWifiHandler(void *pvParameters)
{
  (void) pvParameters;
  Serial.println("Wifi Task Started");
  
  Serial.print(F("\nStarting ConfigOnDoubleReset with DoubleResetDetect using ")); Serial.print(FS_Name);
  Serial.print(F(" on ")); Serial.println(ARDUINO_BOARD);
  Serial.println(ESP_WIFIMANAGER_VERSION);
  Serial.println(ESP_DOUBLE_RESET_DETECTOR_VERSION);

  if ( String(ESP_WIFIMANAGER_VERSION) < ESP_WIFIMANAGER_VERSION_MIN_TARGET )
  {
    Serial.print(F("Warning. Must use this example on Version equal or later than : "));
    Serial.println(ESP_WIFIMANAGER_VERSION_MIN_TARGET);
  }

  Serial.setDebugOutput(false);

  if (FORMAT_FILESYSTEM)
    FileFS.format();

  // Format FileFS if not yet
#ifdef ESP32
  if (!FileFS.begin(true))
#else
  if (!FileFS.begin())
#endif
  {
#ifdef ESP8266
    FileFS.format();
#endif

    Serial.println(F("SPIFFS/LittleFS failed! Already tried formatting."));
  
    if (!FileFS.begin())
    {     
      // prevents debug info from the library to hide err message.
      delay(100);
      
#if USE_LITTLEFS
      Serial.println(F("LittleFS failed!. Please use SPIFFS or EEPROM. Stay forever"));
#else
      Serial.println(F("SPIFFS failed!. Please use LittleFS or EEPROM. Stay forever"));
#endif

      while (true)
      {
        delay(1);
      }
    }
  }

  drd = new DoubleResetDetector(DRD_TIMEOUT, DRD_ADDRESS);

  unsigned long startedAt = millis();

  // New in v1.4.0
  initAPIPConfigStruct(WM_AP_IPconfig);
  initSTAIPConfigStruct(WM_STA_IPconfig);
  //////

  //Local intialization. Once its business is done, there is no need to keep it around
  // Use this to default DHCP hostname to ESP8266-XXXXXX or ESP32-XXXXXX
  //ESP_WiFiManager ESP_wifiManager;
  // Use this to personalize DHCP hostname (RFC952 conformed)
  ESP_WiFiManager ESP_wifiManager("ConfigOnDoubleReset");

#if USE_CUSTOM_AP_IP
  //set custom ip for portal
  // New in v1.4.0
  ESP_wifiManager.setAPStaticIPConfig(WM_AP_IPconfig);
  //////
#endif

  ESP_wifiManager.setMinimumSignalQuality(-1);

  // From v1.0.10 only
  // Set config portal channel, default = 1. Use 0 => random channel from 1-13
  ESP_wifiManager.setConfigPortalChannel(0);
  //////

#if !USE_DHCP_IP    
    // Set (static IP, Gateway, Subnetmask, DNS1 and DNS2) or (IP, Gateway, Subnetmask). New in v1.0.5
    // New in v1.4.0
    ESP_wifiManager.setSTAStaticIPConfig(WM_STA_IPconfig);
    //////
#endif

  // New from v1.1.1
#if USING_CORS_FEATURE
  ESP_wifiManager.setCORSHeader("Your Access-Control-Allow-Origin");  
#endif

  // We can't use WiFi.SSID() in ESP32 as it's only valid after connected.
  // SSID and Password stored in ESP32 wifi_ap_record_t and wifi_config_t are also cleared in reboot
  // Have to create a new function to store in EEPROM/SPIFFS for this purpose
  Router_SSID = ESP_wifiManager.WiFi_SSID();
  Router_Pass = ESP_wifiManager.WiFi_Pass();

  //Remove this line if you do not want to see WiFi password printed
  Serial.println("ESP Self-Stored: SSID = " + Router_SSID + ", Pass = " + Router_Pass);

  // SSID/PWD to uppercase
  ssid.toUpperCase();
  password = "My" + ssid;

  bool configDataLoaded = false;

  // From v1.1.0, Don't permit NULL password
  if ( (Router_SSID != "") && (Router_Pass != "") )
  {
    LOGERROR3(F("* Add SSID = "), Router_SSID, F(", PW = "), Router_Pass);
    wifiMulti.addAP(Router_SSID.c_str(), Router_Pass.c_str());

    ESP_wifiManager.setConfigPortalTimeout(120); //If no access point name has been previously entered disable timeout.
    Serial.println(F("Got ESP Self-Stored Credentials. Timeout 120s for Config Portal"));
  }
  
  if (loadConfigData())
  {
    configDataLoaded = true;
    
    ESP_wifiManager.setConfigPortalTimeout(120); //If no access point name has been previously entered disable timeout.
    Serial.println(F("Got stored Credentials. Timeout 120s for Config Portal")); 

#if USE_ESP_WIFIMANAGER_NTP      
    if ( strlen(WM_config.TZ_Name) > 0 )
    {
      LOGERROR3(F("Current TZ_Name ="), WM_config.TZ_Name, F(", TZ = "), WM_config.TZ);

  #if ESP8266
      configTime(WM_config.TZ, "pool.ntp.org"); 
  #else
      //configTzTime(WM_config.TZ, "pool.ntp.org" );
      configTzTime(WM_config.TZ, "0.pool.ntp.org", "1.pool.ntp.org", "2.pool.ntp.org");
  #endif   
    }
    else
    {
      Serial.println(F("Current Timezone is not set. Enter Config Portal to set."));
    } 
#endif
  }
  else
  {
    // Enter CP only if no stored SSID on flash and file
    Serial.println(F("Open Config Portal without Timeout: No stored Credentials."));
    initialConfig = true;
  }

  if (drd->detectDoubleReset())
  {
    // DRD, disable timeout.
    ESP_wifiManager.setConfigPortalTimeout(0);

    Serial.println(F("Open Config Portal without Timeout: Double Reset Detected"));
    initialConfig = true;
  }

  if (initialConfig)
  {
    led_statemachine_status = LED_STATE_CONFIG;
    
    Serial.print(F("Starting configuration portal @ "));
    
#if USE_CUSTOM_AP_IP    
    Serial.print(APStaticIP);
#else
    Serial.print(F("192.168.4.1"));
#endif

    Serial.print(F(", SSID = "));
    Serial.print(ssid);
    Serial.print(F(", PWD = "));
    Serial.println(password);

    digitalWrite(PIN_LED, LED_ON); // turn the LED on by making the voltage LOW to tell us we are in configuration mode.

    //sets timeout in seconds until configuration portal gets turned off.
    //If not specified device will remain in configuration mode until
    //switched off via webserver or device is restarted.
    //ESP_wifiManager.setConfigPortalTimeout(600);

    // Starts an access point
    if (!ESP_wifiManager.startConfigPortal((const char *) ssid.c_str(), (const char *) password.c_str()))
      Serial.println(F("Not connected to WiFi but continuing anyway."));
    else
    {
      Serial.println(F("WiFi connected...yeey :)"));
    }

    // Stored  for later usage, from v1.1.0, but clear first
    memset(&WM_config, 0, sizeof(WM_config));

    for (uint8_t i = 0; i < NUM_WIFI_CREDENTIALS; i++)
    {
      String tempSSID = ESP_wifiManager.getSSID(i);
      String tempPW   = ESP_wifiManager.getPW(i);

      if (strlen(tempSSID.c_str()) < sizeof(WM_config.WiFi_Creds[i].wifi_ssid) - 1)
        strcpy(WM_config.WiFi_Creds[i].wifi_ssid, tempSSID.c_str());
      else
        strncpy(WM_config.WiFi_Creds[i].wifi_ssid, tempSSID.c_str(), sizeof(WM_config.WiFi_Creds[i].wifi_ssid) - 1);

      if (strlen(tempPW.c_str()) < sizeof(WM_config.WiFi_Creds[i].wifi_pw) - 1)
        strcpy(WM_config.WiFi_Creds[i].wifi_pw, tempPW.c_str());
      else
        strncpy(WM_config.WiFi_Creds[i].wifi_pw, tempPW.c_str(), sizeof(WM_config.WiFi_Creds[i].wifi_pw) - 1);

      // Don't permit NULL SSID and password len < MIN_AP_PASSWORD_SIZE (8)
      if ( (String(WM_config.WiFi_Creds[i].wifi_ssid) != "") && (strlen(WM_config.WiFi_Creds[i].wifi_pw) >= MIN_AP_PASSWORD_SIZE) )
      {
        LOGERROR3(F("* Add SSID = "), WM_config.WiFi_Creds[i].wifi_ssid, F(", PW = "), WM_config.WiFi_Creds[i].wifi_pw );
        wifiMulti.addAP(WM_config.WiFi_Creds[i].wifi_ssid, WM_config.WiFi_Creds[i].wifi_pw);
      }
    }

#if USE_ESP_WIFIMANAGER_NTP      
    String tempTZ   = ESP_wifiManager.getTimezoneName();

    if (strlen(tempTZ.c_str()) < sizeof(WM_config.TZ_Name) - 1)
      strcpy(WM_config.TZ_Name, tempTZ.c_str());
    else
      strncpy(WM_config.TZ_Name, tempTZ.c_str(), sizeof(WM_config.TZ_Name) - 1);

    const char * TZ_Result = ESP_wifiManager.getTZ(WM_config.TZ_Name);
    
    if (strlen(TZ_Result) < sizeof(WM_config.TZ) - 1)
      strcpy(WM_config.TZ, TZ_Result);
    else
      strncpy(WM_config.TZ, TZ_Result, sizeof(WM_config.TZ_Name) - 1);
         
    if ( strlen(WM_config.TZ_Name) > 0 )
    {
      LOGERROR3(F("Saving current TZ_Name ="), WM_config.TZ_Name, F(", TZ = "), WM_config.TZ);

  #if ESP8266
      configTime(WM_config.TZ, "pool.ntp.org"); 
  #else
      //configTzTime(WM_config.TZ, "pool.ntp.org" );
      configTzTime(WM_config.TZ, "0.pool.ntp.org", "1.pool.ntp.org", "2.pool.ntp.org");
  #endif
    }
    else
    {
      LOGERROR(F("Current Timezone Name is not set. Enter Config Portal to set."));
    }
#endif

    // New in v1.4.0
    ESP_wifiManager.getSTAStaticIPConfig(WM_STA_IPconfig);
    //////
    
    saveConfigData();
  }

  digitalWrite(PIN_LED, LED_OFF); // Turn led off as we are not in configuration mode.

  startedAt = millis();

  if (!initialConfig)
  {
    // Load stored data, the addAP ready for MultiWiFi reconnection
    if (!configDataLoaded)
      loadConfigData();

    for (uint8_t i = 0; i < NUM_WIFI_CREDENTIALS; i++)
    {
      // Don't permit NULL SSID and password len < MIN_AP_PASSWORD_SIZE (8)
      if ( (String(WM_config.WiFi_Creds[i].wifi_ssid) != "") && (strlen(WM_config.WiFi_Creds[i].wifi_pw) >= MIN_AP_PASSWORD_SIZE) )
      {
        LOGERROR3(F("* Add SSID = "), WM_config.WiFi_Creds[i].wifi_ssid, F(", PW = "), WM_config.WiFi_Creds[i].wifi_pw );
        wifiMulti.addAP(WM_config.WiFi_Creds[i].wifi_ssid, WM_config.WiFi_Creds[i].wifi_pw);
      }
    }

    if ( WiFi.status() != WL_CONNECTED ) 
    {
      Serial.println(F("ConnectMultiWiFi in setup"));
     
      connectMultiWiFi();
    }
  }

  Serial.print(F("After waiting "));
  Serial.print((float) (millis() - startedAt) / 1000);
  Serial.print(F(" secs more in setup(), connection result is "));

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print(F("connected. Local IP: "));
    Serial.println(WiFi.localIP());
    led_statemachine_status = LED_STATE_NORMAL;
  }
  else {
    led_statemachine_status = LED_STATE_NOCONNECTION;
    Serial.println(ESP_wifiManager.getStatus(WiFi.status()));
  }

  for (;;) {
    drd->loop();
    check_status();
    delay(1000);
  }
}
