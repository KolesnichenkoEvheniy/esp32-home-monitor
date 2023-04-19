#include "globals.h"

// DHT Sensor
DHTesp dht;

// co2 sensoor
Adafruit_CCS811 ccs;

// Temperature sensor
Generic_LM75 temperature(TEMRATURE_LM75_ADDR);

LiquidCrystal_I2C lcd(LCD_ADDR,16,2);

int refreshInterval = 30;

unsigned long previousMillis = 0;

bool showDebugLight = false;

float averageAdcValue(int pin, int n = 5) {
  float sum = 0;

  for(int i=0; i < n; i++) {
    sum += analogRead(pin);
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }

  return sum / n;
}