#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include "globals.h"
#include "network.h"
#include "metrics.h"
#include "files.h"
#include "measurements.h"
#include "config.h"

TimerHandle_t tmr;
int timerId = 1;

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

void ping( TimerHandle_t xTimer )
{
  showDebugLight = false;
  Serial.println("TIMER!");

  lcd.setBacklight((showDebugLight == true) ? HIGH : LOW);
}

void ListenButton( void * pvParameters ){
  Serial.println("Task1 running on core " + String(xPortGetCoreID()));
  
  for(;;){
    if (digitalRead(PIN_BUTTON) == LOW) {
      vTaskDelay(20 / portTICK_PERIOD_MS);
      if (digitalRead(PIN_BUTTON) == LOW) {
        
        toggleDebug(!showDebugLight);
        if (showDebugLight)
        {
          Serial.println("Setting timer...");
          tmr = xTimerCreate("MyTimer", pdMS_TO_TICKS(DEBUG_OFF_TIMER * 1000), pdFALSE, ( void * )timerId, &ping);
          if( xTimerStart(tmr, 10 ) != pdPASS ) {
            Serial.println("Timer start error");
          }
        } 
      }
      while (digitalRead(PIN_BUTTON) == LOW);
      vTaskDelay(20 / portTICK_PERIOD_MS);
      while (digitalRead(PIN_BUTTON) == LOW);
    }

    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
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

// ========== MAIN FUNCTIONS: SETUP & LOOP ========== 
// SETUP: Function called at boot to initialize the system
void setup() {
  // Start the serial output at 115,200 baud
  Serial.begin(115200);

  lcd.init();
  initSPIFFS();
  if (!setupNetworkAndServer()) {
    return;
  }
  setupPrometheusClient();
  initESPNowClient();

  // Start the DHT sensor
  dht.setup(PIN_DHT, DHTesp::DHT11);

  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_LIGHT_SENSOR, INPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  lcd.noBacklight();

  Serial.println("CCS811 Reading CO2 and VOC");
  Wire.begin(PIN_SDA, PIN_SCL);
  if(!ccs.begin(CCS811_ADDR)){
    Serial.println("Failed to start sensor! Please check your wiring.");
    while(1);
  } else {
    Serial.println("CCS811 started");
  }
  // Wait for the sensor to be ready
  while(!ccs.available());

  scani2c();
  
  Serial.println("Creating tasks");
  xTaskCreate(ListenButton, "ListenButton", 10000, NULL, 1, NULL); 
  xTaskCreate(PerformMeasurements, "PerformMeasurements", 10000, NULL, 2, NULL);

  Serial.print("Free Mem After Setup: ");
  Serial.println(freeMemory());
 
  delay(500); 
}

// LOOP: Function called in a loop to read from sensors and send them do databases
void loop() {
  
}