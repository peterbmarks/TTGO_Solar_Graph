/*
  Code for a Liligo TTGO ESP32 board
  Gets Space Weather from api
  https://sws-data.sws.bom.gov.au

API Key: f16e0579-472b-4a59-ad97-6a0d57202650

curl -X POST\
  -H "Content_Type: application/json; charset=UTF-8"\
  -d '{"api_key": "f16e0579-472b-4a59-ad97-6a0d57202650", "options": {"location": "Australian region"}}'\
  "https://sws-data.sws.bom.gov.au/api/v1/get-k-index"

  You'll need to install the ESP32 board files.

  The following Libraries will be needed:
  * TFT_eSPI
  * TFT_eWidget
  * ArduinoJson by Benoit

  This source at: https://github.com/peterbmarks/TTGO_Solar_Graph
*/


#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <TFT_eSPI.h>
#include <TFT_eWidget.h>               // Widget library

//#define USE_SERIAL Serial

#include "WifiCredentials.h"
// Create this file with the following:
//const char *kWifiNetwork = "SSID";
//const char *kWifiPassword = "PASSWORD";
// Don't forget to uncomment them in the file

const char * API_KEY = "f16e0579-472b-4a59-ad97-6a0d57202650";

TFT_eSPI tft = TFT_eSPI();
WiFiMulti wifiMulti;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("Starting...");
  wifiMulti.addAP(kWifiNetwork, kWifiPassword);
  
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);

  tft.drawString("Connecting to Wifi...", 0, 10, 4);
}

void loop() {  
// wait for WiFi connection
  if((wifiMulti.run() == WL_CONNECTED)) {
    tft.fillScreen(TFT_BLACK);
    tft.drawString("Connecting to BOM...", 0, 10, 4);
      HTTPClient http;

      // configure traged server and url
      http.begin("https://sws-data.sws.bom.gov.au/api/v1/get-k-index"); //HTTP
      http.addHeader("Content-Type", "application/json");
      // start connection and send HTTP header
      String payload = "{\"api_key\": \"" + String(API_KEY) + "\", \"options\": {\"location\": \"Australian region\"}}";
      Serial.println(payload);
      int httpCode = http.POST(payload);
      Serial.println(httpCode);

      // httpCode will be negative on error
      if(httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          // file found at server
          if(httpCode == HTTP_CODE_OK) {
              String payload = http.getString();
              
              // https://arduinojson.org
              DynamicJsonDocument doc(1024);
              DeserializationError error = deserializeJson(doc, payload);
              // Test if parsing succeeds.
              if (error) {
                Serial.print(F("deserializeJson() failed: "));
                Serial.println(error.f_str());
                return;
              }
              Serial.println("Deserialised OK");
              Serial.println(payload);    
              int datalen = doc["data"].size();
              Serial.printf("Found %d elements in data\n", datalen);
              if(datalen > 0){
                long k_index = doc["data"][0]["index"];
                const char*  analysis_time = doc["data"][0]["analysis_time"];
                Serial.printf("K Index = %ld\n", k_index);
                Serial.printf("analysis time = %s\n", analysis_time);
                Serial.println("Serial print OK");

                tft.fillScreen(TFT_BLACK);
                tft.drawString("K Index    Australia", 0, 10, 4);
                tft.drawString(String(k_index), 90, 32, 8);
                tft.drawString(analysis_time, 0, 120, 2);
              } else {
                tft.fillScreen(TFT_BLACK);
                tft.drawString("No Data", 0, 30, 8);
                Serial.println("Error: no data");
              }
            if (error) {
              Serial.printf("Json error");
            return;
            }

          } else {
            tft.fillScreen(TFT_BLACK);
            tft.drawString("Http error", 10, 10, 4);
            Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
          }
      http.end();
      }
  }
  delay(200000);
}
