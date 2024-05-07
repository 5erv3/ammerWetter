// --------------------------------------------------------------
//
//                       B O O M Y  T E C H  
//
// YouTube: https://www.youtube.com/@boomytech7504
//
// EMail: boomytechgmail.com
//
// --------------------------------------------------------------
// TimeLib
// https://github.com/PaulStoffregen/Time
// NTPClient
// https://github.com/taranais/NTPClient
//
// OPEN METEO
// https://open-meteo.com/en/docs#latitude=45.50&longitude=10.93&hourly=temperature_2m,surface_pressure&start_date=2022-11-29&end_date=2022-11-29
//
// Weather Code
//     0	Clear sky
//     1, 2, 3	  Mainly clear, partly cloudy, and overcast
//     45, 48	    Fog and depositing rime fog
//     51, 53, 55	Drizzle: Light, moderate, and dense intensity
//     56, 57	    Freezing Drizzle: Light and dense intensity
//     61, 63, 65	Rain: Slight, moderate and heavy intensity
//     66, 67	    Freezing Rain: Light and heavy intensity
//     71, 73, 75	Snow fall: Slight, moderate, and heavy intensity
//     77	        Snow grains
//     80, 81, 82	Rain showers: Slight, moderate, and violent
//     85, 86	    Snow showers slight and heavy
//     95 *	      Thunderstorm: Slight or moderate
//     96, 99 *	  Thunderstorm with slight and heavy hail


// SUN                0                   GIALLO 7000
// OVERCAST           1->3                BLU 39800
// FOG                45-->48             GRIGIO DESATURARE TUTTO
// RAIN               61-->63 80-->81     BLU 42000 con effetto
// HEAVY RAIN         65 82               BLU 42000 con effetto x2
// STORM              95                  FUCSIA 55540 con effetto
// HEAVY STORM       96-->99              55540 con effetto x2
// SNOW               71-->77             ROSSO 65000 con effetto
// HEAVY SNOW         86                  65000 effetto x2

// 0  SUNNY
// 1-->3 OVERCAST
// 51-->57 RAIN
// 45-->48 FOG
// 61-->63 RAIN
// 65 --> HEAVY RAIN
// 71-->77 SNOW
// 80-->81 RAIN
// 82 HEAVY RAIN
// 85 SNOW
// 86 HEAVY SNOW
// 95 STORM
// 96-->99 HEAVY STORM



// GET LONGITUDE LATITUDE
// https://www.latlong.net


#include <TimeLib.h> // https://github.com/PaulStoffregen/Time
#include <NTPClient.h> // https://github.com/taranais/NTPClient

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <WiFiUdp.h>


#ifdef __AVR__
 #include <avr/power.h>
 #endif

#define NumeroLEDS 32
#define pin_LEDS 14

Adafruit_NeoPixel pixel = Adafruit_NeoPixel(NumeroLEDS, pin_LEDS, NEO_GRB + NEO_KHZ800);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);


// ----------------------------------------------------
// USER DEFINED DATA
// ----------------------------------------------------
const char*         ssid = "<your_wifi_name>";
const char*     password = "<wifi_password";
String          LATITUDE = "your_latitude";
String         LONGITUDE = "your_longitude";
int                  GMT = +1;
int      ActiveHourStart = 7; // Hour start working in 0:24 format
int        ActiveHourEnd = 22; // Hour stop working in 0:24 format
// ------------------ END -----------------------------

// ----------------------------------------------------
// SYSTEM PARAMETERS
// ----------------------------------------------------
int Getserverinfointerval = 1; // in hours
int DayForecast = 1; // 1=tomorrow 2=day after tomorrow
int MaxValue = 50; // Max Brightness
// ------------------ END -----------------------------

String formattedDate;
String json_array;
unsigned long last_time = 0;
unsigned long timer_delay = 5000; // Get new data every...
int WeatherCODE = 0;
int MatrixTable[32] = { 3,2,1,0,4,5,6,7,8,9,10,20,19,18,17,16,15,14,13,12,11,21,22,23,24,25,26,27,31,30,29,28 };
time_t LastTimeSincro_t;


// DEMO
unsigned long millis_start=0;
int indice = 0;
int MatriceCODE[10] = {0,1,80,61,65,95,96,45,71,86};

  
void setup() {
    pixel.begin();
    Serial.begin(9600);
 initWiFi();
}


void loop() {

    
     // Check if Active or Standby time
    int Hour_Now = hour( time_t(timeClient.getEpochTime()));
    if ( ((Hour_Now < ActiveHourStart) || (Hour_Now >= ActiveHourEnd)) && LastTimeSincro_t != 0) {
        Animation_STANDBY();
        Serial.print("ORA: ");
        Serial.print (Hour_Now);
        return;
    }
  
    unsigned long Diff = timeClient.getEpochTime() - LastTimeSincro_t; 
    if (Diff > (Getserverinfointerval * 3600) || LastTimeSincro_t == 0) {
        // Time to new meteo update
        // --------------------------------
        Animation_UPDATE();
 
        if(WiFi.status()== WL_CONNECTED){
            timeClient.begin();
            timeClient.setTimeOffset(3600*GMT);
            timeClient.update();
            String MeteoAPI = SetupMeteoApi();
            Serial.println("RICHIEDO AL SERVER I DATI");
            json_array = GET_Request(MeteoAPI.c_str());
            JsonCONV();         
         }
    
         else {
            Animation_ERROR_WIFI();
            Serial.println("WiFi Disconnected");
         }

    LastTimeSincro_t = timeClient.getEpochTime();
    
    }

    UpdateColorDisplay();
    
}






String GET_Request(const char* server) {
  HTTPClient http;    
  http.begin(server);
  int httpResponseCode = http.GET();
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.print("HTTP RRESPONSE CODE: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("ERROR CODE: ");
    Serial.println(httpResponseCode);
  }
  http.end();

  return payload;
}


void UpdateColorDisplay() {

    Serial.print("CODICE ELABORATO: "); Serial.println (WeatherCODE);
  
    if (WeatherCODE == 0)                   Animation_SUNNY();
    if (WeatherCODE >=1 && WeatherCODE <=3) Animation_CLOUDY(); 
    if (WeatherCODE >=80 && WeatherCODE <=81 ) Animation_RAIN();
    if (WeatherCODE >=61 && WeatherCODE <=63) Animation_RAIN();
    if (WeatherCODE == 65 || WeatherCODE == 82) Animation_HEAVY_RAIN();
    if (WeatherCODE == 95)                   Animation_STORM();
    if (WeatherCODE >=96 && WeatherCODE <=99) Animation_HEAVY_STORM(); 
    if (WeatherCODE >=45 && WeatherCODE <=48) Animation_FOG(); 
    if (WeatherCODE >=71 && WeatherCODE <=77) Animation_SNOW(); 
    if (WeatherCODE == 86)                   Animation_HEAVY_SNOW();

}


void Animation_SUNNY(){
    //0
    unsigned C_HUE = 7000; int C_SAT = 255; int C_VALUE = MaxValue;
    unsigned E_HUE = 30000; int E_SAT = 255; int E_VALUE = MaxValue/2;
    FillColor("",C_HUE,C_SAT,C_VALUE,E_HUE, E_SAT,E_VALUE);
}
void Animation_CLOUDY(){
    //3
    unsigned C_HUE = 42000; int C_SAT = 200; int C_VALUE = MaxValue/2;
    unsigned E_HUE = 30000; int E_SAT = 255; int E_VALUE = MaxValue/2;
    FillColor("",C_HUE,C_SAT,C_VALUE,E_HUE, E_SAT,E_VALUE);
} 


void Animation_ERROR_WIFI(){
    Serial.println("ANIMAZIONE ERROR WIFI");
    unsigned C_HUE = 0; int C_SAT = 0; int C_VALUE = 0;
    unsigned E_HUE = 30000; int E_SAT = 255; int E_VALUE = MaxValue;   
    String a;
    a =    "0000";
    a=a+  "0000000";
    a=a+ "0000100000";
    a=a+  "0000000";
    a=a+   "0000";
    
  FillColor(a,C_HUE,C_SAT,C_VALUE,E_HUE, E_SAT,E_VALUE);
   delay(500);
// ------------------------------------------------
}
void Animation_STANDBY(){
    Serial.println("ANIMAZIONE STANBY");
    unsigned C_HUE = 0; int C_SAT = 0; int C_VALUE = 0;
    unsigned E_HUE = 42000; int E_SAT = 255; int E_VALUE = 10;   
    String a;
    a =    "0000";
    a=a+  "0000000";
    a=a+ "0000000000";
    a=a+  "0000000";
    a=a+   "0100";
    
  FillColor(a,C_HUE,C_SAT,C_VALUE,E_HUE, E_SAT,E_VALUE);
   delay(2000);
// ------------------------------------------------
}
void Animation_UPDATE() {
    Serial.println("ANIMAZIONE STANBY");
    unsigned C_HUE = 0; int C_SAT = 0; int C_VALUE = 0;
    unsigned E_HUE = 42000; int E_SAT = 255; int E_VALUE = 2;   
    String a;
    a =    "0000";
    a=a+  "0000000";
    a=a+ "1111111111";
    a=a+  "0000000";
    a=a+   "0000";
    
  FillColor(a,C_HUE,C_SAT,C_VALUE,E_HUE, E_SAT,E_VALUE);
   delay(2000);
// ------------------------------------------------
}



void FillColor(String c_String, float C_HUE, int C_SAT, int C_VALUE, float E_HUE, int E_SAT, int E_VALUE){
    if (c_String == "") c_String = "00000000000000000000000000000000";

    for (int t=0;t<=31; t++) {
        if (  c_String.charAt(t) == '0') {
            pixel.setPixelColor(MatrixTable[t], pixel.ColorHSV(C_HUE, C_SAT, C_VALUE));     
        } else {
            pixel.setPixelColor(MatrixTable[t], pixel.ColorHSV(E_HUE, E_SAT, E_VALUE));
        }
        pixel.show();
    }

}


void JsonCONV() {
    DynamicJsonDocument doc(16384); //TO DO
    DeserializationError error = deserializeJson(doc, json_array);

    if (error) {
     Serial.print("deserializeJson() failed: ");
     Serial.println(error.c_str());
     return;
    }

  const char* daily_units_weathercode = doc["daily_units"]["weathercode"]; 
  const char* daily_time_0 = doc["daily"]["time"][0]; 
  WeatherCODE = doc["daily"]["weathercode"][0]; 
  Serial.print("CODICE LETTO: ");
  Serial.println(WeatherCODE);

}


void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}



String SetupMeteoApi() {
    // (LONG) = Longitude
    // (LAT) = Latitude
    // (DATE) = TOMORROW DATE
    
    String Api="https://api.open-meteo.com/v1/forecast?latitude=(LAT)&longitude=(LONG)&daily=weathercode&timezone=auto&start_date=(DATE)&end_date=(DATE)";

    unsigned long Today = timeClient.getEpochTime();
    time_t Today_t = Today;
    const char *str_Today = ctime(&Today_t);

    time_t Tomorrow_t = Today + (DayForecast * 86400);
    const char *domani = ctime(&Tomorrow_t);

    char TomorrowDate[10];
    sprintf(TomorrowDate,"%04u-%02u-%02u ",year(Tomorrow_t),month(Tomorrow_t),day(Tomorrow_t));

    Api.replace("(DATE)",String(TomorrowDate));
    Api.replace("(LONG)", LONGITUDE);
    Api.replace("(LAT)", LATITUDE);

    return Api;
    
}
