#include <stdio.h>
#include <HTTPClient.h>

#define API_ENDPOINT "http://192.168.10.90:8080/sample.json"

typedef struct {
  char* company;
  char* last_update;
  float current_price;
  float price_change;
  float price_history[5];
} StockData;

StockData fetch_stock(String stock_symbol);