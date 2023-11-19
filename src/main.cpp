#include "main.h"
#include "config.h"

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite background = TFT_eSprite(&tft);
TFT_eSprite header = TFT_eSprite(&tft);
TFT_eSprite graph = TFT_eSprite(&tft);
TFT_eSprite price = TFT_eSprite(&tft);
TFT_eSprite lastUpdate = TFT_eSprite(&tft);

wl_status_t wifiState = WL_DISCONNECTED;
StockData stockData;

int32_t lastUpdateTime = 0;
int32_t lastFetchTime = 0;

WiFiUDP ntpUDP;

WiFiManager wifiManager;
WiFiManagerParameter api_token_param("api_token", "API Token", "", 100);
WiFiManagerParameter ntp_server_param("ntp_server", "NTP Server", "pool.ntp.org", 100);

bool shouldSaveConfig = false;
Preferences preferences;
char api_token[100];
char ntp_server[100];

// By default 'pool.ntp.org' is used with 60 seconds update interval and
// no offset
NTPClient timeClient(ntpUDP);
time_t getTimeSync();

void saveConfigCallback() {
  shouldSaveConfig = true;
}

void setup() {
  Serial.begin(115200);
  Serial.write("Starting...\n");
  // wifiManager.resetSettings();

  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&api_token_param);
  wifiManager.addParameter(&ntp_server_param);
  wifiManager.autoConnect("ESP32 Stock Ticker", "tothemoon");

  strcpy(api_token, api_token_param.getValue());
  strcpy(ntp_server, ntp_server_param.getValue());

  preferences.begin("ticker_config", false);


  if (shouldSaveConfig) {
    preferences.putString("api_token", api_token);
    preferences.putString("ntp_server", ntp_server);
  } else {
    strcpy(api_token, preferences.getString("api_token", "\0").c_str());
    strcpy(ntp_server, preferences.getString("ntp_server", "\0").c_str());
  }

  // connectToWifiTask(NULL);
  
  Serial.write("Connected: ");
  if (WiFi.isConnected()) {
    wifiState = WL_CONNECTED;
    Serial.write("true\n");
  } else {
    Serial.write("false\n");
  }

  timeClient.begin();

  Serial.write("Initializing screen...\n");

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

  setSyncProvider(getTimeSync);
  Serial.write("Time sync scheduled\n");

  // xTaskCreatePinnedToCore(connectToWifiTask, "Wifi", 5000, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
  xTaskCreatePinnedToCore(fetchStockTask, "StockFetcher", 5000, (void *)"GOOG", 1, NULL, ARDUINO_RUNNING_CORE);
}

time_t getTimeSync() {
  unsigned long epochTime = timeClient.getEpochTime();
    Serial.write("Current Time: ");
    char buffer [sizeof(unsigned long)*8+1];
    Serial.write(ultoa(epochTime, buffer, 10));
    Serial.write("\n");
    return epochTime;
}

void fetchStockTask(void *parameter) {
  for(;;) {
    if (wifiState == WL_CONNECTED && timeClient.isTimeSet()) {
      unsigned long unix_epoch = timeClient.getEpochTime();
      Serial.write("Fetching stock data...\n");
      Serial.println(unix_epoch);
      Serial.printf("%d-%d-%d\n", year(unix_epoch), month(unix_epoch), day(unix_epoch));
      char end_date[20];
      
      snprintf(end_date, 20, "%d-%d-%d", year(unix_epoch), month(unix_epoch), day(unix_epoch));

      unsigned long start_epoch = unix_epoch - (86400 * 8);
      char start_date[20];
      snprintf(start_date, 20, "%d-%d-%d", year(start_epoch), month(start_epoch), day(start_epoch));
      Serial.printf("start: %s\nend: %s\napi: %s\n", start_date, end_date, api_token);

      if (api_token == NULL) {
        break;
      }

      StockData fetchedData = fetch_stock(api_token, (char *)parameter, start_date, end_date);
      
      Serial.println("Returned from fetch_stock()\n");
      stockData.company = fetchedData.company;
      stockData.current_price = fetchedData.current_price;
      stockData.after_hours_change = fetchedData.after_hours_change;
      stockData.price_change = fetchedData.price_change;
      stockData.last_update = fetchedData.last_update;
      stockData.last_update = fetchedData.last_update;

      size_t history_size = sizeof(stockData.price_history)/sizeof(stockData.price_history[0]);
      for (int i = 0; i < history_size; i++) {
        stockData.price_history[i] = fetchedData.price_history[i];
      }

      lastFetchTime = millis();
      vTaskDelay(60000);
    } else {
      Serial.write("Waiting to initialize...\n");
      Serial.write("Wifi connected: ");
      if (wifiState == WL_CONNECTED) {
        Serial.write("true\n");
      } else {
        Serial.write("false\n");
      }
      Serial.write("Time set: ");
      if (timeClient.isTimeSet()) {
        Serial.write("true\n");
      } else {
        Serial.write("false\n");
      }
      vTaskDelay(5000);
    }
    
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

  header.pushToSprite(&background, 0, 5);
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

    // Serial.println(String(x1));

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

  graph.pushToSprite(&background, 0, 60);
}

void drawCurrentPrice(float currentPrice, float change) {
  // price.fillRoundRect(0, 0, 109, 110, 4, green);
  price.fillRect(0, 0, 109, 110, TFT_BLACK);
  price.loadFont(NotoSansBold15);
  price.setTextColor(TFT_WHITE);
  price.setTextSize(1);
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

  price.pushToSprite(&background, 211, 60);
}

void drawLastUpdate(char *updateMessage) {
  lastUpdate.fillRect(0, 0, 320, 50, TFT_BLACK);
  lastUpdate.loadFont(NotoSansBold15);
  lastUpdate.setTextColor(TFT_WHITE);
  lastUpdate.setTextSize(1);
  lastUpdate.drawString(updateMessage, 10, 30);
  lastUpdate.unloadFont();
  lastUpdate.pushToSprite(&background, 0, 119);
}

void render() {
  if (stockData.company == NULL) {
    drawTitle("No Data");
  } else {
    drawTitle(stockData.company);
    drawGraph(stockData.price_history);
    drawCurrentPrice(stockData.current_price, stockData.price_change);
    drawLastUpdate(stockData.last_update);
  }

  background.pushSprite(0, 0);
}

void loop() {
  background.fillSprite(TFT_BLACK);
  // background.loadFont(NotoSansBold15);

  if (lastUpdateTime < lastFetchTime) {
    render();
    lastUpdateTime = millis();
  }

  timeClient.update();
}
