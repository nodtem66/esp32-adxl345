#include "main.h"

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Wire.begin(21, 22);
  accel = ADXL345();
  delay(300);
  
  while (!accel.EnsureConnected()) {
    Serial.println("Could not connected ADXL345");
    delay(2000);
  }
  
  Serial.println("ADXL345 Connected");
  accel.SetRange(16, true);
  accel.SetOffset(-0.05, 0, 0.125);

  // inititial packet
  init_preference();
  // register event handler
  WiFi.onEvent(WiFiEvent);
  // create RTOS task
  vTaskPrioritySet(NULL, 1);
  loop_task = xTaskGetCurrentTaskHandle();
  xTaskCreate(LEDTask, "LED", 1000, NULL, 4, NULL);
  xTaskCreate(ATCommandTask, "ATCom", 4096, NULL, 3, NULL);
  //xTaskCreate(WifiTask, "TCP", 4096, NULL, 2, NULL);
  connectToWiFi();
  accel.EnableMeasurements();
}

void loop() {
  if (is_wifi_connected && accel.IsConnected) {
    client.connect(udpAddress.c_str(), udpPort, 1000);
    if (client.connected()) {
      AccelerometerScaled scale = accel.ReadScaledAxis();
      // Serial.printf("%f %f %f (%f)\r\n", scale.XAxis, scale.YAxis, scale.ZAxis, norm(scale));  
      // put the double into 8 bytes
      scale_b64[0].d = scale.XAxis;
      scale_b64[1].d = scale.YAxis;
      scale_b64[2].d = scale.ZAxis;
      // send to host with 8*3 bytes
      client.write((const char*)scale_b64, 24);
      client.stop();
    }
  } else if (!is_wifi_connecting) {
    // Retry to connect WIFI
    while (!is_wifi_connected) {
      connectToWiFi();
      vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
  }
  delay(100);
}

void LEDTask(void *pvParameters) {
  pinMode(LED_BUILTIN, OUTPUT);
  for (;;) {
    digitalWrite(LED_BUILTIN, HIGH);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    digitalWrite(LED_BUILTIN, LOW);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

void WifiTask(void *pvParameters) {
  connectToWiFi();
  while(true) {
    //only send data when connected
    if(is_wifi_connected){
      // send_packet
    } else if (!is_wifi_connecting) {
      // Retry to connect WIFI
      while (!is_wifi_connected) {
        connectToWiFi();
        vTaskDelay(5000 / portTICK_PERIOD_MS);
      }
    }
    //Wait for 0.1 second
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

void ATCommandTask(void *pvParameters)
{
  int state = -1, index_of_equal;
  bool isReboot = false;
  int8_t b;
  String tempStr;
  for(;;) {
    while ((b = Serial.read()) != -1) {
      if (b == 'A') state = 0;
      else if (state == 0 && b =='T') {
        Serial.println("OK");
        // Read String after AT until \n
        String command = Serial.readStringUntil('\n');
        // Find position of = in command string
        index_of_equal = command.indexOf('=');
        if (index_of_equal > 0) {
          // if '=' is found, parse and prepare the input value
          tempStr = command.substring(command.indexOf('=')+1);
          tempStr.replace('\r', '\0');
        }
        preference.begin("ADS1292");
        // ATID, ATID=xxxx 
        // Set device id to xxxx
        if (command.startsWith("ID")) {
          if (index_of_equal > 0 && tempStr.length() > 0) {
            preference.putUChar("ID", (uint8_t) tempStr.toInt());
            isReboot = true;
          } else {
            Serial.printf("%u\r\n", preference.getUChar("ID", 0));
          }
        } 
        // ATSSID, ATSSID=xxxx
        // Set WIFI SSID that want to connect
        else if (command.startsWith("SSID")) {
          if (index_of_equal > 0 && tempStr.length() > 0) {
            preference.putString("SSID", tempStr);
            isReboot = true;
          } else {
            Serial.println(preference.getString("SSID", ""));
          }
        }
        // ATPWD, ATPWD=xxxx
        // Set WIFI Password
        else if (command.startsWith("PWD")) {
          if (index_of_equal > 0 && tempStr.length() > 0) {
            preference.putString("PWD", tempStr);
            isReboot = true;
          } else {
            String pwd = preference.getString("PWD", "");
            for (int i=0; i<pwd.length(); i++) {
              if (i>1 && i!=pwd.length()-1) Serial.print('*');
              else Serial.print(pwd[i]);
            }
            Serial.print("\r\n");
          }
        }
        // ATUDP, ATUDP=0.0.0.0
        // Set IP address of target UDP server
        else if (command.startsWith("UDP")) {
          if (index_of_equal > 0 && tempStr.length() > 0) {
            preference.putString("UDP", tempStr);
            isReboot = true;
          } else {
            Serial.println(preference.getString("UDP", ""));
          }
        }
        // ATPORT, ATPORT=0000
        // Set port of target UDP server
        else if (command.startsWith("PORT")) {
          if (index_of_equal > 0 && tempStr.length() > 0) {
            preference.putUShort("PORT", (uint16_t) tempStr.toInt());
            isReboot = true;
          } else {
            Serial.println(preference.getUShort("PORT", 0));
          }
        }
        preference.end();
        if (isReboot) ESP.restart();
      } else {
        state = -1;
      }
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}