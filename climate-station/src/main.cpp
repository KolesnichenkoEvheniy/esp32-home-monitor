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

void lcdTimerHandle( TimerHandle_t xTimer )
{
  showDebugLight = false;
  Serial.println("LCD TIMER");

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
          tmr = xTimerCreate("MyTimer", pdMS_TO_TICKS(DEBUG_OFF_TIMER * 1000), pdFALSE, ( void * )timerId, &lcdTimerHandle);
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

void measurementsLoop( void * pvParameters ){
  Serial.println("Task2 running on core " + String(xPortGetCoreID()));

  for(;;){
    if(showDebugLight == true) {
      digitalWrite(PIN_LED, HIGH);
    }

    performFreshMeasurements();
    // Check if any reads failed and exit early (to try again).
    if (isnan(measurements.humidity) || isnan(measurements.temperature) || isnan(measurements.eco2) || isnan(measurements.tvoc)) {
      Serial.println(F("Failed to read measurements!"));
      return;
    }

    Serial.println("Temperature: " + String(measurements.temperature) +" Humidity: " + String(measurements.humidity));
    Serial.println("CO2: "+String(measurements.eco2)+"ppm, TVOC: "+String(measurements.tvoc)+"ppb");
    Serial.println("TEMP6000 Sensor readings: Lux=" + String(measurements.ambient_light_lux));

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("CO2:" + String((int)measurements.eco2)+" TCO:"+String((int)measurements.tvoc));
    lcd.setCursor(0,1);
    lcd.print("T:" + String(measurements.temperature) + "H:" + String((int)measurements.humidity) + "L:" + String(measurements.ambient_light_lux));

    logMeasurementMetrics();

    if(showDebugLight == true) {
      vTaskDelay(300 / portTICK_PERIOD_MS);
    }
    digitalWrite(PIN_LED, LOW);
    // wait INTERVAL seconds and then do it again
    vTaskDelay(refreshInterval * 1000 / portTICK_PERIOD_MS);
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

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("SomeESP", "nothingyoucanfindout", 1, 1);
  WiFi.printDiag(Serial);

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
  xTaskCreate(measurementsLoop, "PerformMeasurements", 10000, NULL, 2, &measurementsTaskHandle);
  xTaskCreatePinnedToCore(
    keepWiFiAlive,
    "Keep WIFI alive",
    5000,
    NULL,
    1, 
    NULL,
    CONFIG_ARDUINO_RUNNING_CORE
  );

  initESPNowClient();

  Serial.print("Free Mem After Setup: ");
  Serial.println(freeMemory());
 
  delay(500); 
}

// LOOP: Function called in a loop to read from sensors and send them do databases
void loop() {
  
}