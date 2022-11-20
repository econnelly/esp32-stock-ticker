# ESP32 Stock Ticker

This is a simple project I made to show stock data. It shows the company, last 5 days closing price, current price (or after hours price), and the last update time.

The data comes from a very simple backend API I wrote. It scrapes Yahoo! Finance and puts the data into a JSON file.

Sample output:
```json
{
	"company": "Alphabet Inc. (GOOG)",
	"current_price": 97.8,
	"price_change": -0.7,
	"last_updated": "At close:  04:00PM EST",
	"history": [96.03, 98.72, 98.99, 0.0, 97.8],
	"after_hours_price": 97.57,
	"after_hours_change": -0.23
}
```

![Demo Image](https://imgur.com/hBF9LVD.jpg)
