#include "StockApi.h"
#include "ArduinoJson.h"

StockData fetch_stock(String stock_symbol) {
    DynamicJsonDocument doc(1024);
    HTTPClient client;
    client.begin(API_ENDPOINT);
    int httpCode = client.GET();

    StockData data;

    if (httpCode > 0) {
      String payload = client.getString();
      Serial.println(payload);
      deserializeJson(doc, payload);

      float stock_price = doc["current_price"];
      Serial.println(stock_price);

      data.company = new char[strlen(doc["company"]) + 1];
      strcpy(data.company, doc["company"]);

      if (doc.containsKey("after_hours_price")) {
        data.current_price = doc["after_hours_price"];
      } else {
        data.current_price = doc["current_price"];
      }

      if (doc.containsKey("after_hours_change")) {
        data.after_hours_change = doc["after_hours_change"];
      } else {
        data.price_change = doc["price_change"];
      }

      data.last_update = new char[strlen(doc["last_updated"]) + 1];
      strcpy(data.last_update, doc["last_updated"]);

      for (int i = 0; i < 5; i++) {
        data.price_history[i] = doc["history"][i];
      }
    } else {
        Serial.println("ERROR!");
    }

    return data;
}

