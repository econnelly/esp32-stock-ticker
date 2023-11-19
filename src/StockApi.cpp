#include "StockApi.h"
#include "ArduinoJson.h"

StockData fetch_stock(const char *apiToken, const char *stock_symbol, const char *start_date, const char *end_date) {
    DynamicJsonDocument doc(1024);
    HTTPClient client;
    char apiUrl[1024];
    uint8_t urlLen = strlen(POLYGON_AGGREGATE_API) + strlen(stock_symbol) + strlen(start_date) + strlen(end_date) + strlen(apiToken);
    snprintf(apiUrl, urlLen, POLYGON_AGGREGATE_API, stock_symbol, start_date, end_date, apiToken);
    Serial.println(apiUrl);

    client.begin(apiUrl);
    int httpCode = client.GET();

    StockData data;

    if (httpCode > 0) {
      String payload = client.getString();
      Serial.println(payload);
      deserializeJson(doc, payload);
      Serial.println("Deserialized JSON\n");

      int last_element = doc["queryCount"];
      Serial.printf("Days: %d\n", last_element);
      float stock_price = doc["results"][last_element - 1]["c"];
      data.current_price = stock_price;
      Serial.printf("Stock Price: %.2f\n", stock_price);

      data.company = new char[strlen(stock_symbol) + 1];
      strcpy(data.company, stock_symbol);
      Serial.printf("Company: %s\n", data.company);

      data.after_hours_price = data.current_price;
      data.after_hours_change = 0.0;

      data.last_update = "O";

      Serial.printf("History for the last %d days\n", last_element);
      for (int i = 0; i < last_element; i++) {
        data.price_history[i] = doc["results"][i]["c"];
        Serial.println(data.price_history[i]);
      }

      Serial.printf("After hours price: %d\n", data.after_hours_price);

      // float stock_price = doc["current_price"];
      // Serial.println(stock_price);

      // data.company = new char[strlen(doc["company"]) + 1];
      // strcpy(data.company, doc["company"]);

      // if (doc.containsKey("after_hours_price")) {
      //   data.current_price = doc["after_hours_price"];
      // } else {
      //   data.current_price = doc["current_price"];
      // }

      // if (doc.containsKey("after_hours_change")) {
      //   data.after_hours_change = doc["after_hours_change"];
      // } else {
      //   data.price_change = doc["price_change"];
      // }

      // data.last_update = new char[strlen(doc["last_updated"]) + 1];
      // strcpy(data.last_update, doc["last_updated"]);

      // for (int i = 0; i < 5; i++) {
      //   data.price_history[i] = doc["history"][i];
      // }
    } else {
        Serial.println("ERROR!");
    }

    return data;
}

