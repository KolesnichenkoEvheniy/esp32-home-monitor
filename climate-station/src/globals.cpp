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

TaskHandle_t measurementsTaskHandle = NULL;

float averageAdcValue(int pin, int n = 5) {
  float sum = 0;

  for(int i=0; i < n; i++) {
    sum += analogRead(pin);
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }

  return sum / n;
}

void blink(int count = 2) {
  for(int i = 0; i < count; i++) {
    digitalWrite(PIN_LED, LOW);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    digitalWrite(PIN_LED, HIGH);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    digitalWrite(PIN_LED, LOW);
  }
}

void toggleDebug( bool newDebugState ) {
  showDebugLight = newDebugState;

  lcd.setBacklight((showDebugLight == true) ? HIGH : LOW);
  lcd.noCursor();

  Serial.println("The button is released, state: " + String(showDebugLight));
  blink((showDebugLight == true) ? 3 : 2);
}

void scani2c() {
  byte error, address;
  int nDevices;
  Serial.println("Searching for I2C devices...");
  nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
      nDevices++;
    } else if (error==4) {
      Serial.print("Unknow error at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
  } else {
    Serial.println("done\n");
  }
}