#include <Arduino.h>
#include <Wire.h>
#include <ADXL345.h>
#include <Preferences.h>
#include <WiFi.h>
#include <math.h>

//== ADXL345 variables ===============================
ADXL345 accel;
typedef union {
  double d;
  byte b[8];
} double_bytes;
double_bytes scale_b64[3];
double norm(AccelerometerScaled scale)
{
  return sqrt(pow(scale.XAxis, 2) + pow(scale.YAxis, 2) + pow(scale.ZAxis, 2));
}
//== end ADXL345 variables ===========================

//== WIFI variables ==================================
uint8_t deviceId = 0;
String networkName = "";
String networkPswd = "";
String udpAddress = "192.168.0.1";
uint16_t udpPort = 0;
boolean is_wifi_connected = false;
boolean is_wifi_connecting = false;
WiFiClient client;
Preferences preference;
//== END WIFI variables ===============================

//= = TASKS ============================================
void LEDTask(void *pvParameters);
void WifiTask(void *pvParameters);
void ATCommandTask(void * pvParameters);
TaskHandle_t loop_task = NULL;
//== END TASKS ========================================

// wifi event handler
void WiFiEvent(WiFiEvent_t event) {
  switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      //When connected set 
      Serial.print("WiFi connected! IP address: ");
      Serial.println(WiFi.localIP());  
      //initializes the UDP state
      //This initializes the transfer buffer
      is_wifi_connecting = false;
      is_wifi_connected = true;
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi lost connection");
      is_wifi_connected = false;
      break;
    default:
      break;
  }
}

static inline void connectToWiFi() {
  if (is_wifi_connecting) return;
  is_wifi_connecting = true;
  Serial.println("Connecting to WiFi network: " + networkName);
  // delete old config
  WiFi.disconnect(true);
  //Initiate connection
  WiFi.begin(networkName.c_str(), networkPswd.c_str());
  Serial.println("Waiting for WIFI connection...");
}

static inline void init_preference() {
  preference.begin("ADS1292");
  deviceId = preference.getUChar("ID", 0);
  networkName = preference.getString("SSID", "0000");
  networkPswd = preference.getString("PWD", "");
  udpAddress = preference.getString("UDP", "");
  udpPort = preference.getUShort("PORT", 0);
  preference.end();
}