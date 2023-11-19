#include <stdio.h>
#include <HTTPClient.h>

#define API_ENDPOINT "http://192.168.1.39:8080/stock.json"
#define POLYGON_AGGREGATE_API "https://api.polygon.io/v2/aggs/ticker/%s/range/1/day/%s/%s?adjusted=true&sort=asc&limit=120&apiKey=%s"

typedef struct {
  char* company = NULL;
  char* last_update;
  float current_price;
  float after_hours_price;
  float price_change;
  float after_hours_change;
  float price_history[5];
} StockData;

StockData fetch_stock(const char *apiToken, const char *stock_symbol, const char *start_date, const char *end_date);