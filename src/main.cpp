#include "main.h"

#define WIFI_SSID "****"
#define WIFI_PW "****"

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite background = TFT_eSprite(&tft);
TFT_eSprite header = TFT_eSprite(&tft);
TFT_eSprite graph = TFT_eSprite(&tft);
TFT_eSprite price = TFT_eSprite(&tft);
TFT_eSprite lastUpdate = TFT_eSprite(&tft);

WifiState wifiState = NOT_CONNECTED;
StockData stockData;

void setup() {
  Serial.begin(115200);
  Serial.println("Hello T-Display-S3");

  connectToWifi(NULL);

  stockData = fetch_stock("GOOG");

  // xTaskCreatePinnedToCore(connectToWifi, "Wifi", 5000, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
  xTaskCreatePinnedToCore(fetchStockBySymbol, "StockFetcher", 5000, (void *)"GOOG", 1, NULL, ARDUINO_RUNNING_CORE);

  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(3);

  background.createSprite(320, 170);
  background.setTextDatum(3);
  background.setSwapBytes(true);

  header.createSprite(320, 50);
  header.setSwapBytes(true);

  graph.createSprite(205, 110);
  graph.setSwapBytes(true);

  price.createSprite(109, 110);
  price.setSwapBytes(true);

  lastUpdate.createSprite(320, 50);
  lastUpdate.setSwapBytes(true);
}

void connectToWifi(void *parameter) {
  wifiState = CONNECTING;
  WiFi.begin(WIFI_SSID, WIFI_PW);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  wifiState = CONNECTED;
  Serial.println("Connected to the WiFi network");
}

void fetchStockBySymbol(void *parameter) {
  for(;;) {
    if (wifiState == CONNECTED) {
      stockData = fetch_stock((char *)parameter);
    }
    vTaskDelay(5000);
  }
}

/**
 * Create an array with 3 elements
 * 0 - minimum value in the dataset
 * 1 - average between min and max values
 * 2 - maximum value in the dataset
 */
void getXVals(float data[5], float vals[3]) {
  for (int i = 0; i < 5; i++) {
    if (data[i] > vals[2]) {
      vals[2] = data[i];
    }

    if (vals[0] == 0 || data[i] < vals[0]) {
      vals[0] = data[i];
    }
  }

  vals[1] = (vals[0] + vals[2]) / 2;

  return;
}

void drawTitle(const char *title) {
  header.loadFont(NotoSansBold15);
  header.setTextColor(TFT_WHITE, TFT_BLACK);
  header.fillRect(0, 0, 320, 50, TFT_BLACK);
  // header.fillRoundRect(0, 0, 320, 50, 4, blue);
  header.setTextSize(1);
  header.drawString(title, 10, 15);
  header.unloadFont();
}

/*
 * Calculate how many pixels to move in the y-direction per 1 point change in data
 */
float pixelsPerPoint(float min, float max, int height) {
  float delta = max - min;
  return height / delta;
}

/**
 * Map each data point to a y-value on the sprite
 */
void calculateDataPoints(float rawData[5], float dataPoints[5], int height) {
  float upperAndLower[3] = {0, 0, 0};
  getXVals(rawData, upperAndLower);

  float ppp = pixelsPerPoint(upperAndLower[0], upperAndLower[2], height);

  for (int i = 0; i < 5; i++) {
    dataPoints[i] = height - ((rawData[i] - upperAndLower[0]) * ppp);
  }
}

void drawGraphLines(float data[5], int width, int height, int xOffset, int yOffset) {
  float yvals[5] = {0, 0, 0, 0, 0};
  calculateDataPoints(data, yvals, height);

  float xscale = width / 5.0F;

  for (int i = 1; i < 5; i++) {
    int x1 = xscale * (i - 1) + xOffset;
    int y1 = yvals[i - 1] + yOffset;

    int x2 = xscale * i + xOffset;
    int y2 = yvals[i] + yOffset;

    Serial.println(String(x1));

    graph.drawWideLine(x1, y1, x2, y2, 2, TFT_WHITE);
  }

  return;
}

void drawGraph(float data[5]) {
  float xVals[3] = {0, 0, 0};
  getXVals(data, xVals);
  graph.setTextColor(TFT_WHITE, TFT_BLACK);
  graph.setTextSize(1);
  graph.fillRect(0, 0, 205, 110, TFT_BLACK);
  // graph.fillRoundRect(0, 0, 205, 110, 4, green);
  for (int i = 0; i < 3; i++) {
    graph.drawString(String(xVals[i]), 4, 70 - ((i + 1) * 20));
    for (int j = 25; j < 185; j = j + 5) {
      graph.drawPixel(15 + j, 72 - ((i + 1) * 20), TFT_DARKGREY);
    }
  }

  drawGraphLines(data, 165, 40, 50, 10);
}

void drawCurrentPrice(float currentPrice, float change) {
  // price.fillRoundRect(0, 0, 109, 110, 4, green);
  price.fillRect(0, 0, 109, 110, TFT_BLACK);
  price.loadFont(NotoSansBold15);
  price.setTextColor(TFT_WHITE);
  price.drawString(String(currentPrice), 10, 10);

  if (change > 0) {
    price.setTextColor(TFT_GREEN);
  } else if (change == 0) {
    price.setTextColor(TFT_DARKGREY);
  } else {
    price.setTextColor(TFT_RED);
  }

  price.drawString(String(change), 109 - 10 - 50, 45);
  price.unloadFont();
}

void drawLastUpdate(char *updateMessage) {
  lastUpdate.fillRect(0, 0, 320, 50, TFT_BLACK);
  lastUpdate.loadFont(NotoSansBold15);
  lastUpdate.setTextColor(TFT_WHITE);
  lastUpdate.drawString(updateMessage, 10, 30);
  lastUpdate.unloadFont();
}

void render() {
  header.pushToSprite(&background, 0, 5);
  graph.pushToSprite(&background, 0, 60);
  price.pushToSprite(&background, 211, 60);
  lastUpdate.pushToSprite(&background, 0, 119);

  background.pushSprite(0, 0);
}

void loop() {
  background.fillSprite(TFT_BLACK);
  background.loadFont(NotoSansBold15);

  drawTitle(stockData.company);
  drawGraph(stockData.price_history);
  drawCurrentPrice(stockData.current_price, stockData.price_change);
  drawLastUpdate(stockData.last_update);
  render();
}
