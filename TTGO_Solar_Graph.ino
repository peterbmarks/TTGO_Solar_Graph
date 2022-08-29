
// Demonstrate graph widget functions with two independant trace instances
// Multiple traces can be drawn at a time with multiple trace instances
// Note: Traces are automatically clipped at graph boundaries by widget library

// Requires widget library here:
// https://github.com/Bodmer/TFT_eWidget


#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <TFT_eSPI.h>
#include <TFT_eWidget.h>               // Widget library

#define USE_SERIAL Serial

#include "WifiCredentials.h"
// Create this file with the following:
//const char *kWifiNetwork = "SSID";
//const char *kWifiPassword = "PASSWORD";
// Don't forget to uncomment them in the file

TFT_eSPI tft = TFT_eSPI();
WiFiMulti wifiMulti;

GraphWidget gr = GraphWidget(&tft);    // Graph widget

// Traces are drawn on tft using graph instance
TraceWidget tr1 = TraceWidget(&gr);    // Graph trace 1
TraceWidget tr2 = TraceWidget(&gr);    // Graph trace 2

// Note that the screen is rotated with USB on the left and "TTGO" label on the right
const int kScreenResX = 240;
const int kScreenResY = 135;

const int kXGraphMarginLeft = 30; // fit the legend at left
const int kXGraphMarginRight = 10;
const int kYGraphMarginTop = 5;
const int kYGraphMarginBottom = 15; // fit the legend below

const int kSamples = 100; // number of power samples to keep

const float kXMin = 0.0;
const float kXMax = float(kSamples);
const float kYMin = -10.0;  // minimum power generated
const float kYMax = 3000.0; // maximum power generated

float gGenSamples[kSamples+1];
float gUseSamples[kSamples+1];

void setup() {
  Serial.begin(115200);
  wifiMulti.addAP(kWifiNetwork, kWifiPassword);
  
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);

  // Graph area is 200 pixels wide, 150 high, dark grey background
  gr.createGraph(kScreenResX - kXGraphMarginLeft - kXGraphMarginRight, kScreenResY - kYGraphMarginTop - kYGraphMarginBottom, tft.color565(5, 5, 5));

  // x scale units is from 0 to 100, y scale units is -50 to 50
  gr.setGraphScale(0.0, 100.0, kYMin, kYMax);

  // X grid starts at 0 with lines every 10 x-scale units
  // Y grid starts at -50 with lines every 25 y-scale units
  // blue grid
  gr.setGraphGrid(0.0, 10.0, kYMin, (kYMax - kYMin) / 10.0, TFT_BLUE);

  // Draw empty graph, top left corner at 40,10 on TFT
  gr.drawGraph(kXGraphMarginLeft, kYGraphMarginTop);

  // Draw the x axis scale
  tft.setTextDatum(TC_DATUM); // Top centre text datum
  tft.drawNumber(kXMin, gr.getPointX(0.0), gr.getPointY(kYMin) + 3);
  tft.drawNumber(kXMax / 2, gr.getPointX(kXMax / 2), gr.getPointY(kYMin) + 3);
  tft.drawNumber(kXMax, gr.getPointX(kXMax), gr.getPointY(kYMin) + 3);

  // Draw the y axis scale
  tft.setTextDatum(MR_DATUM); // Middle right text datum
  tft.drawNumber(kYMin, gr.getPointX(0.0), gr.getPointY(kYMin));
  tft.drawNumber(0, gr.getPointX(0.0), gr.getPointY(0.0));
  tft.drawNumber(kYMax, gr.getPointX(0.0), gr.getPointY(kYMax));

  // Restart traces with new colours
  tr1.startTrace(TFT_WHITE);
  tr2.startTrace(TFT_YELLOW);

  clearSamples();
}

void clearSamples() {
  for(int i = 0; i < kSamples; i++) {
    gGenSamples[i] = 0.0;
    gUseSamples[i] = 0.0;
  }
}

void addSample(float gen, float use) {
  slideArrayBack();
  //USE_SERIAL.printf("gen = %f\n", value);
  gGenSamples[kSamples] = gen;
  gUseSamples[kSamples] = use;
}

void slideArrayBack() {
  for(int i = 0; i < kSamples; i++) {
    gGenSamples[i] = gGenSamples[i+1];
    gUseSamples[i] = gUseSamples[i+1];
  }
}

void plotData() {
  // Draw empty graph at 40,10 on display
  gr.drawGraph(kXGraphMarginLeft, kYGraphMarginTop);
  // Start new trace
  tr1.startTrace(TFT_GREEN);
  tr2.startTrace(TFT_RED);

  for(int i = 0; i < kSamples; i++) {
    tr1.addPoint(i, gGenSamples[i]);
    tr2.addPoint(i, gUseSamples[i]);
    //USE_SERIAL.printf("plot x = %d, y = %f\n", i, value); 
  }
}

void loop() {  
// wait for WiFi connection
  if((wifiMulti.run() == WL_CONNECTED)) {
      HTTPClient http;

      // configure traged server and url
      http.begin("http://envoy.local/production.json"); //HTTP

      // start connection and send HTTP header
      int httpCode = http.GET();

      // httpCode will be negative on error
      if(httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          // file found at server
          if(httpCode == HTTP_CODE_OK) {
              String payload = http.getString();
              // https://arduinojson.org
              DynamicJsonDocument doc(3072);
              DeserializationError error = deserializeJson(doc, payload);

            if (error) {
            USE_SERIAL.printf("Json error");
            return;
          }

            // Extract current power generated from the Envoy controller JSON

            long genWattsNow = doc["production"][1]["wNow"];
            long useWattsNow = doc["consumption"][0]["wNow"];
            addSample(genWattsNow, useWattsNow);
          }
      } else {
          USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }

      http.end();
  }
  plotData();
  delay(5000);
}
