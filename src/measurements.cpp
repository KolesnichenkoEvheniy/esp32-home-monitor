#include "measurements.h"

int loopCounter = 0;

void PerformMeasurements( void * pvParameters ){
  Serial.println("Task2 running on core " + String(xPortGetCoreID()));

  for(;;){
    if(showDebugLight == true) {
      digitalWrite(PIN_LED, HIGH);
    }

    int64_t time;
    time = transport.getTimeMillis();

    // Read temperature and humidity
    flag:TempAndHumidity newValues = dht.getTempAndHumidity(); //Get the Temperature and humidity
    if (dht.getStatus() != 0) { //Judge if the correct value is read
      vTaskDelay(100 / portTICK_PERIOD_MS);
      goto flag; //If there is an error, go back to the flag and re-read the data
    }

    float hum = newValues.humidity;
    float cels = temperature.readTemperatureC();
    float eCO2, tvoc;

    // Compute heat index in Celsius (isFahreheit = false)
    float hic = dht.computeHeatIndex(cels, hum, false);

    if(ccs.available()){
      ccs.setEnvironmentalData(hum, cels);
      uint8_t ccsData = ccs.readData();
      Serial.println("CCS811 Sensor readings: " + String(ccsData));
      eCO2 = ccs.geteCO2();
      tvoc = ccs.getTVOC();
    }

    // light
    float ambientLightReadingAdc = averageAdcValue(PIN_LIGHT_SENSOR, 5);
    double ambientLightVoltage = ambientLightReadingAdc / 4095.0 * 3.3;
    float amps = ambientLightVoltage / 10000.0; // across 10,000 Ohms
    float microamps = amps * 1000000;
    int ambientLightLux = (int)(microamps * 2.0);

    Serial.println("TEMP6000 Sensor readings: ADC=" + String(ambientLightReadingAdc) + "Lux=" + String(ambientLightLux));

    // Check if any reads failed and exit early (to try again).
    if (isnan(hum) || isnan(cels) || isnan(eCO2) || isnan(tvoc)) {
      Serial.println(F("Failed to read from DHT sensor!"));
      return;
    }

    Serial.println("Temperature: " + String(cels) +" Humidity: " + String(hum));
    Serial.println("CO2: "+String(eCO2)+"ppm, TVOC: "+String(tvoc)+"ppb");

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("CO2:" + String((int)eCO2)+" TCO:"+String((int)tvoc));
    lcd.setCursor(0,1);
    lcd.print("T:" + String(cels) + "H:" + String((int)hum) + "L:" + String(ambientLightLux));

    if (loopCounter >= 5) {
      Serial.println("Sending samples...");
      //Send
      loopCounter = 0;
      PromClient::SendResult res = client.send(req);
      if (!res == PromClient::SendResult::SUCCESS) {
          Serial.println(client.errmsg);
      }
      Serial.println("Samples sent.");
      
      // Reset batches after a succesful send.
      ts1.resetSamples();
      ts2.resetSamples();
      ts3.resetSamples();
      ts4.resetSamples();
      ts5.resetSamples();
      ts6.resetSamples();
      ts7.resetSamples();
    } else {
      if (!ts1.addSample(time, cels)) {
        Serial.println(ts1.errmsg);
      }
      if (!ts2.addSample(time, hum)) {
        Serial.println(ts2.errmsg);
      }
      if (!ts3.addSample(time, hic)) {
        Serial.println(ts3.errmsg);
      }
      if (!ts4.addSample(time, WiFi.RSSI())) {
        Serial.println(ts4.errmsg);
      }
      if (!ts5.addSample(time, eCO2)) {
        Serial.println(ts5.errmsg);
      }
      if (!ts6.addSample(time, tvoc)) {
        Serial.println(ts6.errmsg);
      }
      if (!ts7.addSample(time, ambientLightLux)) {
        Serial.println(ts7.errmsg);
      }
      loopCounter++;
    }

    //bluetoothClientLoop();
    ///////// ---
    // Serial.println("Show light? " + String(showDebugLight));

    if(showDebugLight == true) {
      vTaskDelay(300 / portTICK_PERIOD_MS);
    }
    digitalWrite(PIN_LED, LOW);
    // wait INTERVAL seconds and then do it again
    vTaskDelay(refreshInterval * 1000 / portTICK_PERIOD_MS);
  }
}