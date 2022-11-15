#include <Arduino.h>
#include <TFT_eSPI.h>
#include <WiFi.h>

#include "NotoSansBold15.h"
#include "NotoSansBold36.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "StockApi.h"

enum WifiState {
    NOT_CONNECTED,
    CONNECTING,
    CONNECTED
};

void getXVals(float data[5], float vals[3]);
void drawTitle(const char* title);
float pixelsPerPoint(float min, float max, int height);
void calculateDataPoints(float rawData[5], float dataPoints[5], int height);
void drawGraphLines(float data[5], int width, int height, int xOffset, int yOffset);
void drawGraph(float data[5]);
void drawCurrentPrice(float currentPrice, float change);
void drawLastUpdate(char *updateMessage);
void render();
void fetchStockBySymbol(void *parameter);
void connectToWifi(void *parameter);